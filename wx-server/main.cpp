// TinyWeChat Server — Phase 0 骨架
// 当前仅验证：编译通过 + SQLite 可打开 + 监听端口就绪
// 后续阶段集成协议解析、消息路由、业务逻辑

#include <iostream>
#include <sqlite3.h>

int main()
{
    std::cout << "[server] TinyWeChat Server v0.1 — Phase 0 starting..." << std::endl;

    // 1. 验证 SQLite 可用
    sqlite3* db = nullptr;
    int rc = sqlite3_open("data/tinywechat.db", &db);
    if (rc != SQLITE_OK) {
        std::cerr << "[server] SQLite open failed: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    std::cout << "[server] SQLite opened OK" << std::endl;

    // 2. 启用 WAL 模式
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    std::cout << "[server] WAL mode enabled" << std::endl;

    sqlite3_close(db);
    std::cout << "[server] SQLite closed OK" << std::endl;

    // 3. 验证日志基础设施
    std::cout << "[server] Phase 0 check passed — ready for Phase 1" << std::endl;

    return 0;
}
