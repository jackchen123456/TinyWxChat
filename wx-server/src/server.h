#pragma once

#include <string>
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

class Database;
class Session;

/// Maximum concurrent connections (M-3: DoS protection)
constexpr int MAX_CONNECTIONS = 256;

/// Central server: owns the listening socket, session registry, and database.
class Server {
public:
    Server();
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /// Initialise database, bind, and start listening.
    /// Returns false on fatal error.
    bool init(const std::string& dbPath, const std::string& bindAddr, int port);

    /// Blocking accept loop. Returns when stop() is called.
    void run();

    /// Signal the accept loop to exit.
    void stop();

    // ── Session registry (thread-safe) ───────────────────
    // CRITICAL-3: registerSession now takes shared_ptr to share ownership.
    // findSession returns shared_ptr so callers get a safe reference.
    void registerSession(int64_t userId, std::shared_ptr<Session> session);
    void unregisterSession(Session* session);

    /// Find the session belonging to userId, or nullptr.  Returns shared_ptr
    /// so the caller can safely use the session without it being destroyed.
    std::shared_ptr<Session> findSession(int64_t userId);

    /// Kick any existing session for userId (called on new login).
    void kickExisting(int64_t userId);

    // ── All sessions management ─────────────────────────
    void addSession(std::shared_ptr<Session> session);
    void removeSession(Session* session);
    std::shared_ptr<Session> getSession(Session* session);

    // ── Database access ──────────────────────────────────
    Database& db() { return *db_; }

    // ── Logging ──────────────────────────────────────────
    void log(const std::string& msg);

    // ── Connection count (for M-3 limit) ─────────────────
    int connectionCount() const { return connectionCount_.load(); }

private:
    void acceptLoop();

    int listenFd_ = -1;
    std::atomic<bool> running_{true};

    std::unique_ptr<Database> db_;

    // session map: user_id -> shared_ptr<Session> (shared ownership)
    std::mutex              sessionsMutex_;
    std::unordered_map<int64_t, std::shared_ptr<Session>> sessions_;
    
    // all sessions (for lifecycle management)
    std::mutex                          allSessionsMutex_;
    std::vector<std::shared_ptr<Session>> allSessions_;

    // M-3: connection counter for limit enforcement
    std::atomic<int> connectionCount_{0};
};
