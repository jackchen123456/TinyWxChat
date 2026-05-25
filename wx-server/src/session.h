#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <nlohmann/json_fwd.hpp>

class Server;

// ── Heartbeat timeout (CRITICAL-1) ──────────────────────
// Server closes connection if no data received for this many milliseconds.
constexpr int HEARTBEAT_TIMEOUT_MS = 90 * 1000;  // 90 seconds

// ── Write queue limit (M-2) ─────────────────────────────
// Maximum bytes allowed in the per-session write queue before dropping.
constexpr size_t MAX_WRITE_QUEUE_BYTES = 4 * 1024 * 1024;  // 4 MiB（可缓存几条图片消息）

// ── Session state ────────────────────────────────────────
enum class SessionState {
    CONNECTED,   // TCP established, not yet logged in
    LOGGED_IN,   // auth.login succeeded
    CLOSING,     // being torn down
};

/// Per-connection session.  Owns the socket and runs a read-loop in a thread.
class Session {
public:
    Session(int fd, Server& server);
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    // ── Identity (set after login) ───────────────────────
    int64_t userId()   const { return userId_; }
    const std::string& nickname() const { return nickname_; }

    void setLoggedIn(int64_t uid, const std::string& nick);

    // ── Send ─────────────────────────────────────────────
    /// Thread-safe: queue bytes to be sent on the socket.
    /// Returns false if the write queue is full (M-2).
    bool sendBytes(const std::vector<uint8_t>& data);

    // ── Lifecycle ────────────────────────────────────────
    void start();
    /// Signal the session to stop. Safe to call from any thread.
    void shutdown();

    /// Close the underlying socket only (used by kickExisting).
    void closeSocket();

    SessionState state() const { return state_.load(); }

private:
    void readLoop();
    bool readExact(void* buf, size_t n);
    void handleFrame(const uint8_t* header, const std::string& payload);
    void dispatchMessage(const std::string& type, int seq, const nlohmann::json& body);
    void writerLoop();

    // ── Individual message handlers ──────────────────────
    void handleAuthLogin(int seq, const nlohmann::json& body);
    void handleAuthRegister(int seq, const nlohmann::json& body);
    void handleChatSend(int seq, const nlohmann::json& body);
    void handleChatHistory(int seq, const nlohmann::json& body);
    void handleChatConversations(int seq, const nlohmann::json& body);
    void handleFriendRequest(int seq, const nlohmann::json& body);
    void handleFriendHandle(int seq, const nlohmann::json& body);
    void handleFriendList(int seq, const nlohmann::json& body);
    void handleFriendPending(int seq, const nlohmann::json& body);
    void handleGroupCreate(int seq, const nlohmann::json& body);
    void handleGroupJoin(int seq, const nlohmann::json& body);
    void handleGroupSend(int seq, const nlohmann::json& body);
    void handleGroupHistory(int seq, const nlohmann::json& body);
    void handleGroupList(int seq, const nlohmann::json& body);
    void handleGroupApply(int seq, const nlohmann::json& body);
    void handleGroupApplyHandle(int seq, const nlohmann::json& body);

    int         fd_;
    Server&     server_;
    std::atomic<SessionState> state_{SessionState::CONNECTED};

    int64_t     userId_   = 0;
    std::string nickname_;

    // Simple write-queue: mutex + vector + condition_variable
    std::mutex              shutdownMutex_;
    std::mutex              writeMutex_;
    std::condition_variable writeCv_;
    std::vector<uint8_t>    writeQueue_;
    size_t                  writeQueueBytes_ = 0;  // M-2: track queue size

    std::unique_ptr<std::thread> readThread_;
    std::unique_ptr<std::thread> writeThread_;
    std::atomic<bool> running_{true};
};
