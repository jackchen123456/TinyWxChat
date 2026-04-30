#include "session.h"
#include "server.h"
#include "protocol.h"
#include "database.h"

#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <thread>
#include <cstring>

Session::Session(int fd, Server& server)
    : fd_(fd), server_(server)
{
}

Session::~Session()
{
    shutdown();
}

void Session::setLoggedIn(int64_t uid, const std::string& nick)
{
    userId_   = uid;
    nickname_ = nick;
    state_.store(SessionState::LOGGED_IN);
}

void Session::sendBytes(const std::vector<uint8_t>& data)
{
    if (data.empty()) return;
    {
        std::lock_guard<std::mutex> lk(writeMutex_);
        writeQueue_.insert(writeQueue_.end(), data.begin(), data.end());
    }
    writeCv_.notify_one();
}

void Session::start()
{
    readThread_  = std::make_unique<std::thread>(&Session::readLoop,  this);
    writeThread_ = std::make_unique<std::thread>(&Session::writerLoop, this);
}

void Session::shutdown()
{
    {
        std::lock_guard<std::mutex> lk(shutdownMutex_);
        bool expected = true;
        if (!running_.compare_exchange_strong(expected, false))
            return;  // already shutting down
    }

    state_.store(SessionState::CLOSING);
    close(fd_);
    writeCv_.notify_all();

    // Avoid self-join: if shutdown is called from within readThread or
    // writeThread, we must not join that thread (would deadlock).
    std::thread::id self = std::this_thread::get_id();
    if (readThread_ && readThread_->joinable() && readThread_->get_id() != self)
        readThread_->join();
    if (writeThread_ && writeThread_->joinable() && writeThread_->get_id() != self)
        writeThread_->join();

    server_.unregisterSession(this);
}

void Session::closeSocket()
{
    close(fd_);
}

// ── Reader ──────────────────────────────────────────────

void Session::readLoop()
{
    while (running_.load()) {
        // 1) Read 8-byte header
        uint8_t header[FRAME_HEADER_SIZE];
        if (!readExact(header, FRAME_HEADER_SIZE)) break;

        Frame frame;
        if (!parseHeader(header, frame)) {
            server_.log("session: invalid frame header, closing");
            break;
        }

        // 2) Read payload
        std::string payload;
        if (frame.length > 0) {
            payload.resize(frame.length);
            if (!readExact(&payload[0], frame.length)) break;
        }

        // 3) Handle
        if (frame.frameType == FrameType::PINGPONG && payload.empty()) {
            // PING → PONG
            Frame pong;
            pong.frameType = FrameType::PINGPONG;
            sendBytes(serializeFrame(pong));
        } else if (frame.frameType == FrameType::REQUEST ||
                   frame.frameType == FrameType::NOTIFICATION) {
            handleFrame(header, payload);
        } else {
            // unexpected frame type on server side (RESPONSE from client is ignored)
        }
    }

    server_.log("session: read loop ended, shutting down");
    shutdown();
}

bool Session::readExact(void* buf, size_t n)
{
    size_t received = 0;
    while (received < n) {
        ssize_t r = recv(fd_, static_cast<char*>(buf) + received,
                         n - received, 0);
        if (r <= 0) return false;  // EOF or error
        received += static_cast<size_t>(r);
    }
    return true;
}

void Session::handleFrame(const uint8_t* /*header*/, const std::string& payload)
{
    try {
        json j = json::parse(payload);
        std::string type = j.value("type", "");
        int seq = j.value("seq", 0);
        json body = j.value("body", json::object());

        dispatchMessage(type, seq, body);
    } catch (const json::parse_error&) {
        auto err = buildError(ErrCode::INVALID_REQUEST, "Invalid JSON payload");
        sendBytes(serializeFrame(err));
    }
}

void Session::dispatchMessage(const std::string& type, int seq, const json& body)
{
    // ── PING ────────────────────────────────────────────
    if (type == MsgType::PING) {
        Frame pong;
        pong.frameType = FrameType::PINGPONG;
        sendBytes(serializeFrame(pong));
        return;
    }

    // ── auth.login ──────────────────────────────────────
    if (type == MsgType::LOGIN) {
        if (state_.load() == SessionState::LOGGED_IN) {
            // already logged in
            json res;
            res["code"]    = ErrCode::SUCCESS;
            res["user_id"] = userId_;
            res["nickname"] = nickname_;
            sendBytes(serializeFrame(buildResponse(MsgType::LOGIN_RES, seq, res)));
            return;
        }

        std::string username = body.value("username", "");
        std::string password = body.value("password", "");

        auto user = server_.db().authenticate(username, password);
        if (!user) {
            json res;
            res["code"]    = ErrCode::WRONG_PASSWORD;
            res["message"] = "用户名或密码错误";
            sendBytes(serializeFrame(buildResponse(MsgType::LOGIN_RES, seq, res)));
            return;
        }

        // Kick any existing session for this user
        server_.kickExisting(user->id);

        // Mark this session as logged in
        setLoggedIn(user->id, user->nickname);
        server_.registerSession(user->id, this);

        server_.log("login: " + user->username + " (uid=" +
                    std::to_string(user->id) + ")");

        json res;
        res["code"]     = ErrCode::SUCCESS;
        res["user_id"]  = user->id;
        res["nickname"] = user->nickname;
        sendBytes(serializeFrame(buildResponse(MsgType::LOGIN_RES, seq, res)));
        return;
    }

    // ── Check auth for all other messages ───────────────
    if (state_.load() != SessionState::LOGGED_IN) {
        auto err = buildError(ErrCode::UNAUTHORIZED, "请先登录");
        sendBytes(serializeFrame(err));
        return;
    }

    // ── chat.send ──────────────────────────────────────
    if (type == MsgType::SEND) {
        int64_t to_uid  = body.value("to_user_id", int64_t(0));
        std::string content = body.value("content", "");
        int msg_type = body.value("msg_type", 1);

        if (to_uid == 0 || content.empty()) {
            auto err = buildError(ErrCode::INVALID_REQUEST, "缺少 to_user_id 或 content");
            sendBytes(serializeFrame(err));
            return;
        }
        if (content.size() > 4096) {
            auto err = buildError(ErrCode::MESSAGE_TOO_LONG, "消息内容过长（最大4096字节）");
            sendBytes(serializeFrame(err));
            return;
        }

        // Save to DB
        int64_t msg_id = server_.db().saveMessage(userId_, to_uid, content, msg_type);
        int64_t timestamp = static_cast<int64_t>(std::time(nullptr));

        // Send ack to sender
        json ack;
        ack["code"]      = ErrCode::SUCCESS;
        ack["msg_id"]    = msg_id;
        ack["timestamp"] = timestamp;
        sendBytes(serializeFrame(buildResponse(MsgType::SEND_RES, seq, ack)));

        // Forward to recipient if online
        Session* target = server_.findSession(to_uid);
        if (target) {
            json recvBody;
            recvBody["from_user_id"]   = userId_;
            recvBody["from_nickname"]  = nickname_;
            recvBody["msg_id"]         = msg_id;
            recvBody["content"]        = content;
            recvBody["msg_type"]       = msg_type;
            recvBody["timestamp"]      = timestamp;
            auto recv = buildNotification(MsgType::RECV, recvBody);
            target->sendBytes(serializeFrame(recv));
        }
        // If offline, message is already in DB → next history pull will show it
        return;
    }

    // ── chat.history ───────────────────────────────────
    if (type == MsgType::HISTORY) {
        int64_t with_uid      = body.value("with_user_id", int64_t(0));
        int64_t before_msg_id = body.value("before_msg_id", int64_t(0));
        int limit             = body.value("limit", 20);

        if (with_uid == 0) {
            auto err = buildError(ErrCode::INVALID_REQUEST, "缺少 with_user_id");
            sendBytes(serializeFrame(err));
            return;
        }

        auto rows = server_.db().getHistory(userId_, with_uid, before_msg_id, limit);

        json arr = json::array();
        for (auto& r : rows) {
            json m;
            m["msg_id"]       = r.msg_id;
            m["from_user_id"] = r.from_uid;
            m["to_user_id"]   = r.to_uid;
            m["content"]      = r.content;
            m["msg_type"]     = r.msg_type;
            m["timestamp"]    = r.timestamp;
            arr.push_back(m);
        }

        json res;
        res["code"]     = ErrCode::SUCCESS;
        res["messages"] = arr;
        sendBytes(serializeFrame(buildResponse(MsgType::HISTORY_RES, seq, res)));
        return;
    }

    // ── Unknown ────────────────────────────────────────
    auto err = buildError(ErrCode::INVALID_REQUEST,
                          "未知消息类型: " + type);
    sendBytes(serializeFrame(err));
}

// ── Writer ──────────────────────────────────────────────

void Session::writerLoop()
{
    while (running_.load()) {
        std::vector<uint8_t> chunk;
        {
            std::unique_lock<std::mutex> lk(writeMutex_);
            writeCv_.wait(lk, [this] {
                return !writeQueue_.empty() || !running_.load();
            });
            if (!running_.load()) break;
            chunk.swap(writeQueue_);
        }

        size_t sent = 0;
        while (sent < chunk.size() && running_.load()) {
            ssize_t n = send(fd_, chunk.data() + sent, chunk.size() - sent,
                             MSG_NOSIGNAL);
            if (n <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                goto exit_loop;  // connection closed
            }
            sent += static_cast<size_t>(n);
        }
    }
exit_loop:
    // If writer dies, force shutdown
    if (running_.load()) {
        server_.log("session: writer loop ended, shutting down");
        shutdown();
    }
}
