#include "server.h"
#include <iostream>
#include <csignal>
#include <atomic>

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

    // Register signal handler
    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);

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
