#pragma once

#include <string>
#include <vector>
#include <optional>
#include <sqlite3.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct UserInfo {
    int64_t id = 0;
    std::string username;
    std::string nickname;
};

struct MessageRow {
    int64_t msg_id = 0;
    int64_t from_uid = 0;
    int64_t to_uid = 0;
    std::string content;
    int msg_type = 1;     // 1=text
    int64_t timestamp = 0;
};

class Database {
public:
    ~Database();

    /// Open (or create) the database at `path`.  Create tables + seed test users.
    bool open(const std::string& path);

    // ── Auth ─────────────────────────────────────────────
    /// Returns user info if username + password match; empty optional otherwise.
    std::optional<UserInfo> authenticate(const std::string& username,
                                         const std::string& password);

    /// Register a new user. Returns user_id or -1 on failure.
    int64_t registerUser(const std::string& username,
                         const std::string& password,
                         const std::string& nickname);

    /// Look up a user by id.
    std::optional<UserInfo> userById(int64_t uid);

    // ── Messages ─────────────────────────────────────────
    /// Save a single-chat message. Returns generated msg_id.
    int64_t saveMessage(int64_t from_uid, int64_t to_uid,
                        const std::string& content, int msg_type = 1);

    /// Retrieve the most recent `limit` messages between two users.
    /// If before_msg_id == 0, starts from newest; otherwise earlier than that id.
    std::vector<MessageRow> getHistory(int64_t uid_a, int64_t uid_b,
                                       int64_t before_msg_id, int limit);

    /// Get the maximum msg_id (for seq tracking in the sender).
    int64_t maxMsgId();

private:
    void createTables();
    void seedTestUsers();

    sqlite3* db_ = nullptr;

    // Prepared statements (compiled once)
    sqlite3_stmt* stmt_auth_       = nullptr;
    sqlite3_stmt* stmt_insert_msg_ = nullptr;
    sqlite3_stmt* stmt_max_id_     = nullptr;
};
