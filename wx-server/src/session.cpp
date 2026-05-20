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
    // Ensure shutdown was called
    {
        std::lock_guard<std::mutex> lk(shutdownMutex_);
        running_.store(false);
    }
    close(fd_);
    writeCv_.notify_all();

    // Safety net: detach any still-joinable threads to prevent terminate()
    if (readThread_ && readThread_->joinable())
        readThread_->detach();
    if (writeThread_ && writeThread_->joinable())
        writeThread_->detach();
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
    // Capture shared_ptr to keep session alive until threads finish
    auto self = server_.getSession(this);
    if (self) {
        readThread_ = std::make_unique<std::thread>([this, self]() { readLoop(); });
        writeThread_ = std::make_unique<std::thread>([this, self]() { writerLoop(); });
    }
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

    // Join or detach threads.  We must not join ourselves (deadlock),
    // and we must not leave a joinable thread (terminate).
    std::thread::id self = std::this_thread::get_id();
    if (readThread_) {
        if (readThread_->joinable() && readThread_->get_id() != self)
            readThread_->join();
        else if (readThread_->joinable())
            readThread_->detach();
    }
    if (writeThread_) {
        if (writeThread_->joinable() && writeThread_->get_id() != self)
            writeThread_->join();
        else if (writeThread_->joinable())
            writeThread_->detach();
    }

    server_.unregisterSession(this);
    server_.removeSession(this);
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
    // ── PING (no auth required) ─────────────────────────
    if (type == MsgType::PING) {
        Frame pong;
        pong.frameType = FrameType::PINGPONG;
        sendBytes(serializeFrame(pong));
        return;
    }

    // ── Auth handlers (no login required) ───────────────
    if (type == MsgType::LOGIN) {
        handleAuthLogin(seq, body);
        return;
    }
    if (type == MsgType::REGISTER) {
        handleAuthRegister(seq, body);
        return;
    }

    // ── Check auth for all other messages ───────────────
    if (state_.load() != SessionState::LOGGED_IN) {
        auto err = buildError(ErrCode::UNAUTHORIZED, "请先登录");
        sendBytes(serializeFrame(err));
        return;
    }

    // ── Chat handlers ───────────────────────────────────
    if (type == MsgType::SEND) {
        handleChatSend(seq, body);
        return;
    }
    if (type == MsgType::HISTORY) {
        handleChatHistory(seq, body);
        return;
    }
    if (type == MsgType::CONVERSATIONS) {
        handleChatConversations(seq, body);
        return;
    }

    // ── Friend handlers ─────────────────────────────────
    if (type == MsgType::FRIEND_REQUEST) {
        handleFriendRequest(seq, body);
        return;
    }
    if (type == MsgType::FRIEND_HANDLE) {
        handleFriendHandle(seq, body);
        return;
    }
    if (type == MsgType::FRIEND_LIST) {
        handleFriendList(seq, body);
        return;
    }
    if (type == MsgType::FRIEND_PENDING) {
        handleFriendPending(seq, body);
        return;
    }

    // ── Group handlers ──────────────────────────────────
    if (type == MsgType::GROUP_CREATE) {
        handleGroupCreate(seq, body);
        return;
    }
    if (type == MsgType::GROUP_JOIN) {
        handleGroupJoin(seq, body);
        return;
    }
    if (type == MsgType::GROUP_SEND) {
        handleGroupSend(seq, body);
        return;
    }
    if (type == MsgType::GROUP_HISTORY) {
        handleGroupHistory(seq, body);
        return;
    }
    if (type == MsgType::GROUP_LIST) {
        handleGroupList(seq, body);
        return;
    }
    if (type == MsgType::GROUP_APPLY) {
        handleGroupApply(seq, body);
        return;
    }
    if (type == MsgType::GROUP_APPLY_HANDLE) {
        handleGroupApplyHandle(seq, body);
        return;
    }

    // ── Unknown ────────────────────────────────────────
    auto err = buildError(ErrCode::INVALID_REQUEST, "未知消息类型: " + type);
    sendBytes(serializeFrame(err));
}

// ── Individual message handlers ─────────────────────────

void Session::handleAuthLogin(int seq, const json& body)
{
    if (state_.load() == SessionState::LOGGED_IN) {
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

    server_.kickExisting(user->id);
    setLoggedIn(user->id, user->nickname);
    server_.registerSession(user->id, this);

    server_.log("login: " + user->username + " (uid=" + std::to_string(user->id) + ")");

    json res;
    res["code"]     = ErrCode::SUCCESS;
    res["user_id"]  = user->id;
    res["nickname"] = user->nickname;
    sendBytes(serializeFrame(buildResponse(MsgType::LOGIN_RES, seq, res)));
}

void Session::handleAuthRegister(int seq, const json& body)
{
    std::string username = body.value("username", "");
    std::string password = body.value("password", "");
    std::string nickname = body.value("nickname", username);

    if (username.empty() || password.empty()) {
        json res;
        res["code"]    = ErrCode::INVALID_REQUEST;
        res["message"] = "用户名和密码不能为空";
        sendBytes(serializeFrame(buildResponse(MsgType::REGISTER_RES, seq, res)));
        return;
    }
    if (username.size() > 32) {
        json res;
        res["code"]    = ErrCode::USERNAME_TOO_LONG;
        res["message"] = "用户名最长 32 字节";
        sendBytes(serializeFrame(buildResponse(MsgType::REGISTER_RES, seq, res)));
        return;
    }
    if (password.size() < 6) {
        json res;
        res["code"]    = ErrCode::INVALID_REQUEST;
        res["message"] = "密码最少 6 位";
        sendBytes(serializeFrame(buildResponse(MsgType::REGISTER_RES, seq, res)));
        return;
    }

    int64_t uid = server_.db().registerUser(username, password, nickname);
    if (uid < 0) {
        json res;
        res["code"]    = ErrCode::DUPLICATE_USERNAME;
        res["message"] = "用户名已被注册";
        sendBytes(serializeFrame(buildResponse(MsgType::REGISTER_RES, seq, res)));
        return;
    }

    server_.log("register: " + username + " (uid=" + std::to_string(uid) + ")");
    json res;
    res["code"]    = ErrCode::SUCCESS;
    res["user_id"] = uid;
    sendBytes(serializeFrame(buildResponse(MsgType::REGISTER_RES, seq, res)));
}

void Session::handleChatSend(int seq, const json& body)
{
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

    int64_t msg_id = server_.db().saveMessage(userId_, to_uid, content, msg_type);
    int64_t timestamp = static_cast<int64_t>(std::time(nullptr));

    json ack;
    ack["code"]      = ErrCode::SUCCESS;
    ack["msg_id"]    = msg_id;
    ack["timestamp"] = timestamp;
    sendBytes(serializeFrame(buildResponse(MsgType::SEND_RES, seq, ack)));

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
}

void Session::handleChatHistory(int seq, const json& body)
{
    int64_t with_uid      = body.value("with_user_id", int64_t(0));
    int64_t before_msg_id = body.value("before_msg_id", int64_t(0));
    int limit             = body.value("limit", 20);

    if (with_uid == 0) {
        auto err = buildError(ErrCode::INVALID_REQUEST, "缺少 with_user_id");
        sendBytes(serializeFrame(err));
        return;
    }

    auto rows = server_.db().getHistory(userId_, with_uid, before_msg_id, limit);
    auto withUser = server_.db().userById(with_uid);
    std::string withNick = withUser ? withUser->nickname : "未知";

    json arr = json::array();
    for (auto& r : rows) {
        json m;
        m["msg_id"]       = r.msg_id;
        m["from_user_id"] = r.from_uid;
        m["to_user_id"]   = r.to_uid;
        m["from_nickname"] = (r.from_uid == userId_) ? nickname_ : withNick;
        m["to_nickname"]   = (r.to_uid == userId_) ? nickname_ : withNick;
        m["content"]      = r.content;
        m["msg_type"]     = r.msg_type;
        m["timestamp"]    = r.timestamp;
        arr.push_back(m);
    }

    json res;
    res["code"]     = ErrCode::SUCCESS;
    res["messages"] = arr;
    sendBytes(serializeFrame(buildResponse(MsgType::HISTORY_RES, seq, res)));
}

void Session::handleChatConversations(int seq, const json& /*body*/)
{
    auto convs = server_.db().getConversations(userId_);
    json arr = json::array();
    for (auto& c : convs) {
        json item;
        item["peer_id"]       = c.peer_id;
        item["peer_nickname"] = c.peer_nickname;
        item["last_content"]  = c.last_content;
        item["last_timestamp"] = c.last_timestamp;
        arr.push_back(item);
    }
    json res;
    res["code"]          = ErrCode::SUCCESS;
    res["conversations"] = arr;
    sendBytes(serializeFrame(buildResponse(MsgType::CONVERSATIONS_RES, seq, res)));
}

void Session::handleFriendRequest(int seq, const json& body)
{
    int64_t to_uid = body.value("to_user_id", int64_t(0));
    std::string msg = body.value("message", "");

    if (to_uid == 0) {
        auto err = buildError(ErrCode::INVALID_REQUEST, "缺少 to_user_id");
        sendBytes(serializeFrame(err));
        return;
    }
    if (to_uid == userId_) {
        json res;
        res["code"] = ErrCode::INVALID_REQUEST;
        sendBytes(serializeFrame(buildResponse(MsgType::FRIEND_REQUEST_RES, seq, res)));
        return;
    }
    if (!server_.db().userById(to_uid)) {
        json res;
        res["code"] = ErrCode::USER_NOT_FOUND;
        sendBytes(serializeFrame(buildResponse(MsgType::FRIEND_REQUEST_RES, seq, res)));
        return;
    }

    auto friends = server_.db().getFriendList(userId_);
    for (auto& f : friends) {
        if (f.user_id == to_uid) {
            json res;
            res["code"] = ErrCode::FRIEND_ALREADY;
            sendBytes(serializeFrame(buildResponse(MsgType::FRIEND_REQUEST_RES, seq, res)));
            return;
        }
    }

    auto pending = server_.db().getPendingRequests(to_uid);
    for (auto& r : pending) {
        if (r.from_uid == userId_) {
            json res;
            res["code"] = ErrCode::FRIEND_REQUEST_EXISTS;
            sendBytes(serializeFrame(buildResponse(MsgType::FRIEND_REQUEST_RES, seq, res)));
            return;
        }
    }

    int64_t reqId = server_.db().createFriendRequest(userId_, to_uid, msg);
    json res;
    res["code"] = ErrCode::SUCCESS;
    sendBytes(serializeFrame(buildResponse(MsgType::FRIEND_REQUEST_RES, seq, res)));

    Session* target = server_.findSession(to_uid);
    if (target) {
        json notifyBody;
        notifyBody["request_id"]    = reqId;
        notifyBody["from_user_id"]  = userId_;
        notifyBody["from_nickname"] = nickname_;
        notifyBody["message"]       = msg;
        notifyBody["timestamp"]     = static_cast<int64_t>(std::time(nullptr));
        auto notify = buildNotification(MsgType::FRIEND_NOTIFY, notifyBody);
        target->sendBytes(serializeFrame(notify));
    }
}

void Session::handleFriendHandle(int seq, const json& body)
{
    int64_t req_id = body.value("request_id", int64_t(0));
    std::string action = body.value("action", "");

    if (action != "accept" && action != "reject") {
        json res;
        res["code"] = ErrCode::INVALID_REQUEST;
        sendBytes(serializeFrame(buildResponse(MsgType::FRIEND_HANDLE_RES, seq, res)));
        return;
    }

    bool ok = server_.db().handleFriendRequest(req_id, action);
    json res;
    res["code"] = ok ? ErrCode::SUCCESS : ErrCode::INVALID_REQUEST;
    sendBytes(serializeFrame(buildResponse(MsgType::FRIEND_HANDLE_RES, seq, res)));
}

void Session::handleFriendList(int seq, const json& /*body*/)
{
    auto friends = server_.db().getFriendList(userId_);
    json arr = json::array();
    for (auto& f : friends) {
        json item;
        item["user_id"]  = f.user_id;
        item["nickname"] = f.nickname;
        arr.push_back(item);
    }
    json res;
    res["code"]    = ErrCode::SUCCESS;
    res["friends"] = arr;
    sendBytes(serializeFrame(buildResponse(MsgType::FRIEND_LIST_RES, seq, res)));
}

void Session::handleFriendPending(int seq, const json& /*body*/)
{
    auto pending = server_.db().getPendingRequests(userId_);
    json arr = json::array();
    for (auto& r : pending) {
        json item;
        item["request_id"]    = r.id;
        item["from_user_id"]  = r.from_uid;
        item["from_nickname"] = r.from_nickname;
        item["message"]       = r.message;
        item["timestamp"]     = r.created_at;
        arr.push_back(item);
    }
    json res;
    res["code"]     = ErrCode::SUCCESS;
    res["requests"] = arr;
    sendBytes(serializeFrame(buildResponse(MsgType::FRIEND_PENDING_RES, seq, res)));
}

void Session::handleGroupCreate(int seq, const json& body)
{
    std::string name = body.value("name", "");
    if (name.empty()) {
        json res;
        res["code"] = ErrCode::INVALID_REQUEST;
        sendBytes(serializeFrame(buildResponse(MsgType::GROUP_CREATE_RES, seq, res)));
        return;
    }

    int64_t gid = server_.db().createGroup(userId_, name);
    server_.log("group.create: " + name + " (gid=" + std::to_string(gid) + ")");
    json res;
    res["code"]     = ErrCode::SUCCESS;
    res["group_id"] = gid;
    sendBytes(serializeFrame(buildResponse(MsgType::GROUP_CREATE_RES, seq, res)));
}

void Session::handleGroupJoin(int seq, const json& body)
{
    int64_t group_id = body.value("group_id", int64_t(0));
    if (group_id == 0) {
        json res;
        res["code"] = ErrCode::INVALID_REQUEST;
        sendBytes(serializeFrame(buildResponse(MsgType::GROUP_JOIN_RES, seq, res)));
        return;
    }

    if (server_.db().isGroupMember(group_id, userId_)) {
        json res;
        res["code"] = ErrCode::GROUP_ALREADY_MEMBER;
        sendBytes(serializeFrame(buildResponse(MsgType::GROUP_JOIN_RES, seq, res)));
        return;
    }

    bool ok = server_.db().joinGroup(group_id, userId_);
    json res;
    res["code"] = ok ? ErrCode::SUCCESS : ErrCode::GROUP_NOT_FOUND;
    sendBytes(serializeFrame(buildResponse(MsgType::GROUP_JOIN_RES, seq, res)));
}

void Session::handleGroupSend(int seq, const json& body)
{
    int64_t group_id = body.value("group_id", int64_t(0));
    std::string content = body.value("content", "");
    int msg_type = body.value("msg_type", 1);

    if (group_id == 0 || content.empty()) {
        auto err = buildError(ErrCode::INVALID_REQUEST, "缺少 group_id 或 content");
        sendBytes(serializeFrame(err));
        return;
    }
    if (content.size() > 4096) {
        auto err = buildError(ErrCode::MESSAGE_TOO_LONG, "消息内容过长");
        sendBytes(serializeFrame(err));
        return;
    }
    if (!server_.db().isGroupMember(group_id, userId_)) {
        auto err = buildError(ErrCode::GROUP_PERMISSION_DENIED, "你不是群成员");
        sendBytes(serializeFrame(err));
        return;
    }

    int64_t msg_seq = server_.db().sendGroupMessage(group_id, userId_, content, msg_type);
    int64_t timestamp = static_cast<int64_t>(std::time(nullptr));

    json ack;
    ack["code"]      = ErrCode::SUCCESS;
    ack["msg_seq"]   = msg_seq;
    ack["timestamp"] = timestamp;
    sendBytes(serializeFrame(buildResponse(MsgType::GROUP_SEND_RES, seq, ack)));

    auto members = server_.db().getGroupMembers(group_id);
    for (auto& m : members) {
        if (m.user_id == userId_) continue;
        Session* target = server_.findSession(m.user_id);
        if (target) {
            json recvBody;
            recvBody["group_id"]       = group_id;
            recvBody["from_user_id"]   = userId_;
            recvBody["from_nickname"]  = nickname_;
            recvBody["msg_seq"]        = msg_seq;
            recvBody["content"]        = content;
            recvBody["msg_type"]       = msg_type;
            recvBody["timestamp"]      = timestamp;
            auto recv = buildNotification(MsgType::GROUP_RECV, recvBody);
            target->sendBytes(serializeFrame(recv));
        }
    }
}

void Session::handleGroupHistory(int seq, const json& body)
{
    int64_t group_id       = body.value("group_id", int64_t(0));
    int64_t before_msg_seq = body.value("before_msg_seq", int64_t(0));
    int limit              = body.value("limit", 20);

    if (group_id == 0) {
        auto err = buildError(ErrCode::INVALID_REQUEST, "缺少 group_id");
        sendBytes(serializeFrame(err));
        return;
    }

    auto rows = server_.db().getGroupHistory(group_id, before_msg_seq, limit);
    json arr = json::array();
    for (auto& r : rows) {
        json m;
        m["group_id"]       = r.group_id;
        m["msg_seq"]        = r.msg_seq;
        m["from_user_id"]   = r.from_uid;
        m["from_nickname"]  = r.from_nickname;
        m["content"]        = r.content;
        m["msg_type"]       = r.msg_type;
        m["timestamp"]      = r.timestamp;
        arr.push_back(m);
    }
    json res;
    res["code"]     = ErrCode::SUCCESS;
    res["messages"] = arr;
    sendBytes(serializeFrame(buildResponse(MsgType::GROUP_HISTORY_RES, seq, res)));
}

void Session::handleGroupList(int seq, const json& /*body*/)
{
    auto groups = server_.db().getUserGroups(userId_);
    json arr = json::array();
    for (auto& [gid, gname] : groups) {
        json item;
        item["group_id"]   = gid;
        item["group_name"] = gname;
        arr.push_back(item);
    }
    json res;
    res["code"]   = ErrCode::SUCCESS;
    res["groups"] = arr;
    sendBytes(serializeFrame(buildResponse(MsgType::GROUP_LIST_RES, seq, res)));
}

void Session::handleGroupApply(int seq, const json& body)
{
    int64_t group_id = body.value("group_id", int64_t(0));
    std::string msg = body.value("message", "");

    if (group_id == 0) {
        auto err = buildError(ErrCode::INVALID_REQUEST, "缺少 group_id");
        sendBytes(serializeFrame(err));
        return;
    }
    if (server_.db().isGroupMember(group_id, userId_)) {
        json res;
        res["code"] = ErrCode::GROUP_ALREADY_MEMBER;
        sendBytes(serializeFrame(buildResponse(MsgType::GROUP_APPLY_RES, seq, res)));
        return;
    }

    int64_t reqId = server_.db().createGroupApply(group_id, userId_, msg);
    if (reqId < 0) {
        json res;
        res["code"] = ErrCode::GROUP_NOT_FOUND;
        sendBytes(serializeFrame(buildResponse(MsgType::GROUP_APPLY_RES, seq, res)));
        return;
    }

    json res;
    res["code"] = ErrCode::SUCCESS;
    sendBytes(serializeFrame(buildResponse(MsgType::GROUP_APPLY_RES, seq, res)));

    int64_t owner_uid = server_.db().getGroupOwner(group_id);
    if (owner_uid > 0 && owner_uid != userId_) {
        Session* owner = server_.findSession(owner_uid);
        if (owner) {
            json notifyBody;
            notifyBody["request_id"]    = reqId;
            notifyBody["group_id"]      = group_id;
            notifyBody["from_user_id"]  = userId_;
            notifyBody["from_nickname"] = nickname_;
            notifyBody["message"]       = msg;
            notifyBody["timestamp"]     = static_cast<int64_t>(std::time(nullptr));
            auto notify = buildNotification(MsgType::GROUP_APPLY_NOTIFY, notifyBody);
            owner->sendBytes(serializeFrame(notify));
        }
    }
}

void Session::handleGroupApplyHandle(int seq, const json& body)
{
    int64_t req_id = body.value("request_id", int64_t(0));
    std::string action = body.value("action", "");

    if (action != "accept" && action != "reject") {
        json res;
        res["code"] = ErrCode::INVALID_REQUEST;
        sendBytes(serializeFrame(buildResponse(MsgType::GROUP_APPLY_HANDLE_RES, seq, res)));
        return;
    }

    int64_t group_id = server_.db().getGroupRequestGroupId(req_id);
    if (group_id < 0) {
        json res;
        res["code"] = ErrCode::INVALID_REQUEST;
        sendBytes(serializeFrame(buildResponse(MsgType::GROUP_APPLY_HANDLE_RES, seq, res)));
        return;
    }

    auto members = server_.db().getGroupMembers(group_id);
    bool authorized = false;
    for (const auto& m : members) {
        if (m.user_id == userId_ && m.role >= 1) {
            authorized = true;
            break;
        }
    }
    if (!authorized) {
        json res;
        res["code"] = ErrCode::GROUP_PERMISSION_DENIED;
        sendBytes(serializeFrame(buildResponse(MsgType::GROUP_APPLY_HANDLE_RES, seq, res)));
        return;
    }

    bool ok = server_.db().handleGroupApply(req_id, action);
    json res;
    res["code"] = ok ? ErrCode::SUCCESS : ErrCode::INVALID_REQUEST;
    sendBytes(serializeFrame(buildResponse(MsgType::GROUP_APPLY_HANDLE_RES, seq, res)));
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
