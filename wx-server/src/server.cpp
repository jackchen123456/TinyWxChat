#include "server.h"
#include "database.h"
#include "session.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>   // TCP_NODELAY
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <iostream>
#include <thread>
#include <cstring>
#include <algorithm>

Server::Server()  = default;
Server::~Server() { stop(); }

bool Server::init(const std::string& dbPath, const std::string& bindAddr, int port)
{
    // ── Database ────────────────────────────────────────
    db_ = std::make_unique<Database>();
    if (!db_->open(dbPath)) {
        std::cerr << "[SRV] Database init failed" << std::endl;
        return false;
    }
    log("Database opened: " + dbPath);

    // ── Socket ──────────────────────────────────────────
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        std::cerr << "[SRV] socket() failed: " << strerror(errno) << std::endl;
        return false;
    }

    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(static_cast<uint16_t>(port));
    if (bindAddr == "0.0.0.0" || bindAddr.empty()) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, bindAddr.c_str(), &addr.sin_addr);
    }

    if (bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[SRV] bind() failed: " << strerror(errno) << std::endl;
        close(listenFd_);
        return false;
    }

    if (listen(listenFd_, SOMAXCONN) < 0) {
        std::cerr << "[SRV] listen() failed: " << strerror(errno) << std::endl;
        close(listenFd_);
        return false;
    }

    log("Listening on " + bindAddr + ":" + std::to_string(port));
    return true;
}

void Server::run()
{
    log("Server started, accepting connections...");
    acceptLoop();
}

void Server::stop()
{
    running_.store(false);
    if (listenFd_ >= 0) {
        // Shutdown the listen socket to unblock accept()
        shutdown(listenFd_, SHUT_RDWR);
    }
}

// ── Accept loop ──────────────────────────────────────────

void Server::acceptLoop()
{
    while (running_.load()) {
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        int clientFd = accept(listenFd_,
                              reinterpret_cast<sockaddr*>(&clientAddr),
                              &addrLen);
        if (clientFd < 0) {
            if (!running_.load()) break;
            std::cerr << "[SRV] accept() failed: " << strerror(errno) << std::endl;
            continue;
        }

        // M-3: Connection limit check
        if (connectionCount_.load() >= MAX_CONNECTIONS) {
            log("Connection limit reached (" + std::to_string(MAX_CONNECTIONS) +
                "), rejecting new connection");
            close(clientFd);
            continue;
        }

        // L-2: Set TCP_NODELAY for low-latency small messages
        int nodelay = 1;
        setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

        char ipBuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuf, sizeof(ipBuf));
        log("New connection: " + std::string(ipBuf) + ":" +
            std::to_string(ntohs(clientAddr.sin_port)));

        // Create session and run it in a dedicated thread
        auto sess = std::make_shared<Session>(clientFd, *this);
        addSession(sess);
        connectionCount_.fetch_add(1);
        sess->start();   // starts read + write threads internally
    }
    log("Accept loop ended");
}

// ── Session registry ─────────────────────────────────────
// CRITICAL-3: registerSession now takes shared_ptr, sharing ownership
// with allSessions_.  No more no-op deleter hack.

void Server::registerSession(int64_t userId, std::shared_ptr<Session> session)
{
    std::lock_guard<std::mutex> lk(sessionsMutex_);
    sessions_[userId] = std::move(session);
}

void Server::unregisterSession(Session* session)
{
    std::lock_guard<std::mutex> lk(sessionsMutex_);
    auto it = sessions_.find(session->userId());
    if (it != sessions_.end() && it->second.get() == session)
        sessions_.erase(it);
}

// CRITICAL-3: findSession returns shared_ptr so callers hold a safe reference.
std::shared_ptr<Session> Server::findSession(int64_t userId)
{
    std::lock_guard<std::mutex> lk(sessionsMutex_);
    auto it = sessions_.find(userId);
    return (it != sessions_.end()) ? it->second : nullptr;
}

void Server::kickExisting(int64_t userId)
{
    std::shared_ptr<Session> old;
    {
        std::lock_guard<std::mutex> lk(sessionsMutex_);
        auto it = sessions_.find(userId);
        if (it != sessions_.end()) {
            old = it->second;
            sessions_.erase(it);
        }
    }
    if (old) {
        log("Kicking old session for uid=" + std::to_string(userId));
        old->closeSocket();
    }
}

// ── Logging ─────────────────────────────────────────────

void Server::log(const std::string& msg)
{
    std::cout << "[SRV] " << msg << std::endl;
}

// ── All sessions management ─────────────────────────────

void Server::addSession(std::shared_ptr<Session> session)
{
    std::lock_guard<std::mutex> lk(allSessionsMutex_);
    allSessions_.push_back(session);
}

void Server::removeSession(Session* session)
{
    std::lock_guard<std::mutex> lk(allSessionsMutex_);
    allSessions_.erase(
        std::remove_if(allSessions_.begin(), allSessions_.end(),
            [session](const std::shared_ptr<Session>& s) {
                return s.get() == session;
            }),
        allSessions_.end());
    // M-3: decrement connection count
    connectionCount_.fetch_sub(1);
}

std::shared_ptr<Session> Server::getSession(Session* session)
{
    std::lock_guard<std::mutex> lk(allSessionsMutex_);
    for (const auto& s : allSessions_) {
        if (s.get() == session) {
            return s;
        }
    }
    return nullptr;
}
