#pragma once

#include <string>
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

class Database;
class Session;

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
    void registerSession(int64_t userId, Session* session);
    void unregisterSession(Session* session);

    /// Find the session belonging to userId, or nullptr.
    Session* findSession(int64_t userId);

    /// Kick any existing session for userId (called on new login).
    void kickExisting(int64_t userId);

    // ── Database access ──────────────────────────────────
    Database& db() { return *db_; }

    // ── Logging ──────────────────────────────────────────
    void log(const std::string& msg);

private:
    void acceptLoop();

    int listenFd_ = -1;
    std::atomic<bool> running_{true};

    std::unique_ptr<Database> db_;

    // session map: user_id -> Session*
    std::mutex              sessionsMutex_;
    std::unordered_map<int64_t, Session*> sessions_;
};
