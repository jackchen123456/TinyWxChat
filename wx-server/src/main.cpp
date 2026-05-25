#include "server.h"
#include <iostream>
#include <csignal>
#include <atomic>
#include <filesystem>

static std::atomic<Server*> g_server{nullptr};

static void signalHandler(int /*sig*/)
{
    if (g_server.load()) {
        g_server.load()->log("Received signal, shutting down...");
        g_server.load()->stop();
    }
}

int main(int argc, char* argv[])
{
    // Parse command line
    std::string bindAddr = "0.0.0.0";
    int port = 9090;
    std::string dbPath = "data/tinywechat.db";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--db" && i + 1 < argc) {
            dbPath = argv[++i];
        } else if (arg == "--bind" && i + 1 < argc) {
            bindAddr = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: tinywechat-server [--port 9090] [--db data/tinywechat.db] [--bind 0.0.0.0]\n";
            return 0;
        }
    }

    std::cout << "=== TinyWeChat Server v0.1 ===" << std::endl;

    // 自动创建数据库所在目录，避免 SQLite "unable to open database file" 报错
    auto dbDir = std::filesystem::path(dbPath).parent_path();
    if (!dbDir.empty() && !std::filesystem::exists(dbDir)) {
        std::error_code ec;
        std::filesystem::create_directories(dbDir, ec);
        if (ec) {
            std::cerr << "Failed to create db directory: " << dbDir << " — " << ec.message() << std::endl;
            return 1;
        }
        std::cout << "[SRV] Created db directory: " << dbDir << std::endl;
    }

    // L-1: Use sigaction() instead of signal() for reliable signal handling
    struct sigaction sa{};
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  // no SA_RESTART — we want accept() to be interrupted
    sigaction(SIGINT,  &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    // Ignore SIGPIPE — write errors handled per-connection via MSG_NOSIGNAL
    struct sigaction sa_ignore{};
    sa_ignore.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa_ignore, nullptr);

    // Create and start server
    Server server;
    g_server.store(&server);

    if (!server.init(dbPath, bindAddr, port)) {
        std::cerr << "Server initialization failed" << std::endl;
        return 1;
    }

    server.run();

    std::cout << "Server stopped." << std::endl;
    return 0;
}
