#include "database.h"
#include <iostream>
#include <crypt.h>
#include <cstring>
#include <ctime>
#include <algorithm>
#include <cstdlib>

// ── bcrypt helpers ──────────────────────────────────────

static std::string bcryptHash(const std::string& password)
{
    // Generate random salt
    static const char* salt_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";
    char salt[22];
    for (int i = 0; i < 22; i++) {
        salt[i] = salt_chars[rand() % 64];
    }
    salt[22] = '\0';
    
    // Build salt string: $2b$12$<22 chars>
    char salt_str[30];
    snprintf(salt_str, sizeof(salt_str), "$2b$12$%s", salt);
    
    struct crypt_data data{};
    char* result = crypt_r(password.c_str(), salt_str, &data);
    if (!result) {
        std::cerr << "[DB] bcrypt hash failed" << std::endl;
        return "";
    }
    return std::string(result);
}

static bool bcryptVerify(const std::string& password, const std::string& hash)
{
    struct crypt_data data{};
    char* result = crypt_r(password.c_str(), hash.c_str(), &data);
    return result && (hash == result);
}

// ── Database ────────────────────────────────────────────

Database::~Database()
{
    if (stmt_auth_)       sqlite3_finalize(stmt_auth_);
    if (stmt_insert_msg_) sqlite3_finalize(stmt_insert_msg_);
    if (stmt_max_id_)     sqlite3_finalize(stmt_max_id_);
    if (db_)              sqlite3_close(db_);
}

bool Database::open(const std::string& path)
{
    srand(time(nullptr));
    
    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[DB] Failed to open: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;",   nullptr, nullptr, nullptr);

    createTables();
    seedTestUsers();

    sqlite3_prepare_v2(db_,
        "SELECT id, username, nickname, password FROM users WHERE username = ?",
        -1, &stmt_auth_, nullptr);

    sqlite3_prepare_v2(db_,
        "INSERT INTO messages (from_uid, to_uid, content, msg_type, timestamp) "
        "VALUES (?, ?, ?, ?, ?)",
        -1, &stmt_insert_msg_, nullptr);

    sqlite3_prepare_v2(db_,
        "SELECT COALESCE(MAX(msg_id), 0) FROM messages",
        -1, &stmt_max_id_, nullptr);

    return true;
}

void Database::createTables()
{
    const char* users_sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            username    TEXT    NOT NULL UNIQUE,
            password    TEXT    NOT NULL,
            nickname    TEXT    NOT NULL DEFAULT '',
            avatar_path TEXT    DEFAULT NULL,
            created_at  INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
    )";

    const char* msgs_sql = R"(
        CREATE TABLE IF NOT EXISTS messages (
            msg_id      INTEGER PRIMARY KEY AUTOINCREMENT,
            from_uid    INTEGER NOT NULL,
            to_uid      INTEGER NOT NULL,
            content     TEXT    NOT NULL,
            msg_type    INTEGER NOT NULL DEFAULT 1,
            extra       TEXT    DEFAULT NULL,
            timestamp   INTEGER NOT NULL,
            FOREIGN KEY (from_uid) REFERENCES users(id),
            FOREIGN KEY (to_uid)   REFERENCES users(id)
        );
        CREATE INDEX IF NOT EXISTS idx_messages_pair ON messages(from_uid, to_uid);
        CREATE INDEX IF NOT EXISTS idx_messages_to   ON messages(to_uid, timestamp DESC);
    )";

    // ── Phase 2: Friends ────────────────────────────────
    const char* friends_sql = R"(
        CREATE TABLE IF NOT EXISTS friends (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            user_a      INTEGER NOT NULL,
            user_b      INTEGER NOT NULL,
            created_at  INTEGER NOT NULL,
            FOREIGN KEY (user_a) REFERENCES users(id),
            FOREIGN KEY (user_b) REFERENCES users(id)
        );
        CREATE UNIQUE INDEX IF NOT EXISTS idx_friends_pair
            ON friends( MIN(user_a, user_b), MAX(user_a, user_b) );

        CREATE TABLE IF NOT EXISTS friend_requests (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            from_uid    INTEGER NOT NULL,
            to_uid      INTEGER NOT NULL,
            message     TEXT    DEFAULT '',
            status      INTEGER NOT NULL DEFAULT 0,
            created_at  INTEGER NOT NULL,
            handled_at  INTEGER DEFAULT NULL,
            FOREIGN KEY (from_uid) REFERENCES users(id),
            FOREIGN KEY (to_uid)   REFERENCES users(id)
        );
        CREATE INDEX IF NOT EXISTS idx_fr_to ON friend_requests(to_uid, status);
    )";

    // ── Phase 3: Groups ─────────────────────────────────
    const char* groups_sql = R"(
        CREATE TABLE IF NOT EXISTS groups (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            name        TEXT    NOT NULL,
            owner_uid   INTEGER NOT NULL,
            avatar_path TEXT    DEFAULT NULL,
            created_at  INTEGER NOT NULL,
            FOREIGN KEY (owner_uid) REFERENCES users(id)
        );

        CREATE TABLE IF NOT EXISTS group_members (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            group_id    INTEGER NOT NULL,
            user_id     INTEGER NOT NULL,
            role        INTEGER NOT NULL DEFAULT 0,
            joined_at   INTEGER NOT NULL,
            FOREIGN KEY (group_id) REFERENCES groups(id),
            FOREIGN KEY (user_id)  REFERENCES users(id),
            UNIQUE(group_id, user_id)
        );
        CREATE INDEX IF NOT EXISTS idx_gm_group ON group_members(group_id);
        CREATE INDEX IF NOT EXISTS idx_gm_user  ON group_members(user_id);

        CREATE TABLE IF NOT EXISTS group_messages (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            group_id    INTEGER NOT NULL,
            msg_seq     INTEGER NOT NULL,
            from_uid    INTEGER NOT NULL,
            content     TEXT    NOT NULL,
            msg_type    INTEGER NOT NULL DEFAULT 1,
            extra       TEXT    DEFAULT NULL,
            timestamp   INTEGER NOT NULL,
            FOREIGN KEY (group_id) REFERENCES groups(id),
            FOREIGN KEY (from_uid)  REFERENCES users(id)
        );
        CREATE UNIQUE INDEX IF NOT EXISTS idx_gm_seq ON group_messages(group_id, msg_seq);

        CREATE TABLE IF NOT EXISTS group_requests (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            group_id    INTEGER NOT NULL,
            from_uid    INTEGER NOT NULL,
            message     TEXT    DEFAULT '',
            status      INTEGER NOT NULL DEFAULT 0,
            created_at  INTEGER NOT NULL,
            handled_at  INTEGER DEFAULT NULL,
            FOREIGN KEY (group_id) REFERENCES groups(id),
            FOREIGN KEY (from_uid) REFERENCES users(id)
        );
        CREATE INDEX IF NOT EXISTS idx_gr_group ON group_requests(group_id, status);
    )";

    char* err = nullptr;
    sqlite3_exec(db_, users_sql,   nullptr, nullptr, &err);
    if (err) { std::cerr << "[DB] users: " << err << std::endl; sqlite3_free(err); }
    sqlite3_exec(db_, msgs_sql,    nullptr, nullptr, &err);
    if (err) { std::cerr << "[DB] messages: " << err << std::endl; sqlite3_free(err); }
    sqlite3_exec(db_, friends_sql, nullptr, nullptr, &err);
    if (err) { std::cerr << "[DB] friends: " << err << std::endl; sqlite3_free(err); }
    sqlite3_exec(db_, groups_sql,  nullptr, nullptr, &err);
    if (err) { std::cerr << "[DB] groups: " << err << std::endl; sqlite3_free(err); }
}

void Database::seedTestUsers()
{
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM users", -1, &stmt, nullptr);
    sqlite3_step(stmt);
    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (count > 0) return;

    std::cout << "[DB] Seeding test users alice / bob / carol (password: 123456) ..." << std::endl;
    registerUser("alice", "123456", "Alice");
    registerUser("bob",   "123456", "Bob");
    registerUser("carol", "123456", "Carol");
}

// ── Auth ────────────────────────────────────────────────

std::optional<UserInfo> Database::authenticate(const std::string& username,
                                                const std::string& password)
{
    sqlite3_reset(stmt_auth_);
    sqlite3_clear_bindings(stmt_auth_);
    sqlite3_bind_text(stmt_auth_, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt_auth_) != SQLITE_ROW)
        return std::nullopt;

    UserInfo u;
    u.id       = sqlite3_column_int64(stmt_auth_, 0);
    u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt_auth_, 1));
    u.nickname = reinterpret_cast<const char*>(sqlite3_column_text(stmt_auth_, 2));
    const char* hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt_auth_, 3));

    if (!bcryptVerify(password, hash))
        return std::nullopt;

    return u;
}

int64_t Database::registerUser(const std::string& username,
                               const std::string& password,
                               const std::string& nickname)
{
    std::string hash = bcryptHash(password);
    if (hash.empty()) return -1;

    const char* sql = R"(
        INSERT INTO users (username, password, nickname, created_at)
        VALUES (?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hash.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, nickname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, static_cast<int64_t>(std::time(nullptr)));

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "[DB] registerUser failed: " << sqlite3_errmsg(db_) << std::endl;
        return -1;
    }
    return sqlite3_last_insert_rowid(db_);
}

std::optional<UserInfo> Database::userById(int64_t uid)
{
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "SELECT id, username, nickname FROM users WHERE id = ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, uid);

    std::optional<UserInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        UserInfo u;
        u.id       = sqlite3_column_int64(stmt, 0);
        u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.nickname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        result = u;
    }
    sqlite3_finalize(stmt);
    return result;
}

// ── Messages ────────────────────────────────────────────

int64_t Database::saveMessage(int64_t from_uid, int64_t to_uid,
                              const std::string& content, int msg_type)
{
    sqlite3_reset(stmt_insert_msg_);
    sqlite3_clear_bindings(stmt_insert_msg_);

    sqlite3_bind_int64(stmt_insert_msg_, 1, from_uid);
    sqlite3_bind_int64(stmt_insert_msg_, 2, to_uid);
    sqlite3_bind_text(stmt_insert_msg_, 3, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt_insert_msg_,   4, msg_type);
    sqlite3_bind_int64(stmt_insert_msg_, 5, static_cast<int64_t>(std::time(nullptr)));

    if (sqlite3_step(stmt_insert_msg_) != SQLITE_DONE) {
        std::cerr << "[DB] saveMessage failed: " << sqlite3_errmsg(db_) << std::endl;
        return -1;
    }
    return sqlite3_last_insert_rowid(db_);
}

std::vector<MessageRow> Database::getHistory(int64_t uid_a, int64_t uid_b,
                                              int64_t before_msg_id, int limit)
{
    std::string sql = R"(
        SELECT msg_id, from_uid, to_uid, content, msg_type, timestamp
        FROM messages
        WHERE ((from_uid = ? AND to_uid = ?) OR (from_uid = ? AND to_uid = ?))
    )";

    if (before_msg_id > 0)
        sql += " AND msg_id < ?";
    sql += " ORDER BY msg_id DESC LIMIT ?";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    int idx = 1;
    sqlite3_bind_int64(stmt, idx++, uid_a);
    sqlite3_bind_int64(stmt, idx++, uid_b);
    sqlite3_bind_int64(stmt, idx++, uid_b);
    sqlite3_bind_int64(stmt, idx++, uid_a);
    if (before_msg_id > 0)
        sqlite3_bind_int64(stmt, idx++, before_msg_id);
    sqlite3_bind_int(stmt, idx++, limit);

    std::vector<MessageRow> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        MessageRow r;
        r.msg_id    = sqlite3_column_int64(stmt, 0);
        r.from_uid  = sqlite3_column_int64(stmt, 1);
        r.to_uid    = sqlite3_column_int64(stmt, 2);
        r.content   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        r.msg_type  = sqlite3_column_int(stmt, 4);
        r.timestamp = sqlite3_column_int64(stmt, 5);
        rows.push_back(r);
    }
    sqlite3_finalize(stmt);

    std::reverse(rows.begin(), rows.end());
    return rows;
}

std::vector<Database::ConversationRow> Database::getConversations(int64_t uid)
{
    // Per the requirements §0.1 decision #5:
    // SELECT DISTINCT ... GROUP BY peer_id ORDER BY last_time DESC
    const char* sql = R"(
        SELECT
            CASE WHEN from_uid = ? THEN to_uid ELSE from_uid END AS peer_id,
            u.nickname,
            content,
            MAX(timestamp) AS last_ts
        FROM messages m
        JOIN users u ON u.id = CASE WHEN from_uid = ? THEN to_uid ELSE from_uid END
        WHERE from_uid = ? OR to_uid = ?
        GROUP BY peer_id
        ORDER BY last_ts DESC
    )";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, uid);
    sqlite3_bind_int64(stmt, 2, uid);
    sqlite3_bind_int64(stmt, 3, uid);
    sqlite3_bind_int64(stmt, 4, uid);

    std::vector<ConversationRow> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ConversationRow c;
        c.peer_id        = sqlite3_column_int64(stmt, 0);
        c.peer_nickname  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        c.last_content   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        c.last_timestamp = sqlite3_column_int64(stmt, 3);
        result.push_back(c);
    }
    sqlite3_finalize(stmt);
    return result;
}

// ── Friends (Phase 2) ──────────────────────────────────

int64_t Database::createFriendRequest(int64_t from_uid, int64_t to_uid,
                                       const std::string& message)
{
    const char* sql = R"(
        INSERT INTO friend_requests (from_uid, to_uid, message, created_at)
        VALUES (?, ?, ?, ?)
    )";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, from_uid);
    sqlite3_bind_int64(stmt, 2, to_uid);
    sqlite3_bind_text(stmt, 3, message.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, static_cast<int64_t>(std::time(nullptr)));

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return -1;
    return sqlite3_last_insert_rowid(db_);
}

bool Database::handleFriendRequest(int64_t request_id, const std::string& action)
{
    int newStatus = (action == "accept") ? 1 : 2;
    int64_t now = static_cast<int64_t>(std::time(nullptr));

    // Get request details first
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "SELECT from_uid, to_uid FROM friend_requests WHERE id = ? AND status = 0",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, request_id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false;
    }
    int64_t from_uid = sqlite3_column_int64(stmt, 0);
    int64_t to_uid   = sqlite3_column_int64(stmt, 1);
    sqlite3_finalize(stmt);

    // Update request status
    sqlite3_prepare_v2(db_,
        "UPDATE friend_requests SET status = ?, handled_at = ? WHERE id = ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, newStatus);
    sqlite3_bind_int64(stmt, 2, now);
    sqlite3_bind_int64(stmt, 3, request_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // If accepted, insert friend relation (ensure a < b)
    if (newStatus == 1) {
        int64_t a = std::min(from_uid, to_uid);
        int64_t b = std::max(from_uid, to_uid);
        sqlite3_prepare_v2(db_,
            "INSERT OR IGNORE INTO friends (user_a, user_b, created_at) VALUES (?, ?, ?)",
            -1, &stmt, nullptr);
        sqlite3_bind_int64(stmt, 1, a);
        sqlite3_bind_int64(stmt, 2, b);
        sqlite3_bind_int64(stmt, 3, now);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    return true;
}

std::vector<FriendRow> Database::getFriendList(int64_t uid)
{
    std::vector<FriendRow> result;
    const char* sql = R"(
        SELECT u.id, u.nickname FROM friends f
        JOIN users u ON (u.id = CASE WHEN f.user_a = ? THEN f.user_b ELSE f.user_a END)
        WHERE f.user_a = ? OR f.user_b = ?
    )";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, uid);
    sqlite3_bind_int64(stmt, 2, uid);
    sqlite3_bind_int64(stmt, 3, uid);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FriendRow r;
        r.user_id  = sqlite3_column_int64(stmt, 0);
        r.nickname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.push_back(r);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<FriendRequestRow> Database::getPendingRequests(int64_t uid)
{
    std::vector<FriendRequestRow> result;
    const char* sql = R"(
        SELECT fr.id, fr.from_uid, u.nickname, fr.message, fr.status, fr.created_at
        FROM friend_requests fr
        JOIN users u ON u.id = fr.from_uid
        WHERE fr.to_uid = ? AND fr.status = 0
        ORDER BY fr.id ASC
    )";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, uid);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FriendRequestRow r;
        r.id            = sqlite3_column_int64(stmt, 0);
        r.from_uid      = sqlite3_column_int64(stmt, 1);
        r.from_nickname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        r.message       = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        r.status        = sqlite3_column_int(stmt, 4);
        r.created_at    = sqlite3_column_int64(stmt, 5);
        result.push_back(r);
    }
    sqlite3_finalize(stmt);
    return result;
}

// ── Groups (Phase 3) ───────────────────────────────────

int64_t Database::createGroup(int64_t owner_uid, const std::string& name)
{
    int64_t now = static_cast<int64_t>(std::time(nullptr));

    // Insert group
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "INSERT INTO groups (name, owner_uid, created_at) VALUES (?, ?, ?)",
        -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, owner_uid);
    sqlite3_bind_int64(stmt, 3, now);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_finalize(stmt);
    int64_t group_id = sqlite3_last_insert_rowid(db_);

    // Owner becomes member with role=2
    sqlite3_prepare_v2(db_,
        "INSERT INTO group_members (group_id, user_id, role, joined_at) VALUES (?, ?, 2, ?)",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, group_id);
    sqlite3_bind_int64(stmt, 2, owner_uid);
    sqlite3_bind_int64(stmt, 3, now);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return group_id;
}

bool Database::joinGroup(int64_t group_id, int64_t user_id)
{
    // First check if group exists
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "SELECT 1 FROM groups WHERE id = ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, group_id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false;  // group doesn't exist
    }
    sqlite3_finalize(stmt);
    
    int64_t now = static_cast<int64_t>(std::time(nullptr));
    sqlite3_prepare_v2(db_,
        "INSERT OR IGNORE INTO group_members (group_id, user_id, role, joined_at) VALUES (?, ?, 0, ?)",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, group_id);
    sqlite3_bind_int64(stmt, 2, user_id);
    sqlite3_bind_int64(stmt, 3, now);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::vector<GroupMemberRow> Database::getGroupMembers(int64_t group_id)
{
    std::vector<GroupMemberRow> result;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "SELECT user_id, role FROM group_members WHERE group_id = ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, group_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GroupMemberRow r;
        r.user_id = sqlite3_column_int64(stmt, 0);
        r.role    = sqlite3_column_int(stmt, 1);
        result.push_back(r);
    }
    sqlite3_finalize(stmt);
    return result;
}

bool Database::isGroupMember(int64_t group_id, int64_t user_id)
{
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "SELECT 1 FROM group_members WHERE group_id = ? AND user_id = ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, group_id);
    sqlite3_bind_int64(stmt, 2, user_id);
    bool ok = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return ok;
}

int64_t Database::getGroupOwner(int64_t group_id)
{
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "SELECT owner_uid FROM groups WHERE id = ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, group_id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return -1;
    }
    int64_t owner_uid = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return owner_uid;
}

std::vector<std::pair<int64_t, std::string>> Database::getUserGroups(int64_t user_id)
{
    std::vector<std::pair<int64_t, std::string>> result;
    const char* sql = R"(
        SELECT g.id, g.name FROM groups g
        JOIN group_members gm ON gm.group_id = g.id
        WHERE gm.user_id = ?
        ORDER BY g.id
    )";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, user_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t gid = sqlite3_column_int64(stmt, 0);
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.emplace_back(gid, name);
    }
    sqlite3_finalize(stmt);
    return result;
}

int64_t Database::sendGroupMessage(int64_t group_id, int64_t from_uid,
                                    const std::string& content, int msg_type)
{
    int64_t now = static_cast<int64_t>(std::time(nullptr));

    // Use a transaction to prevent race condition
    sqlite3_exec(db_, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr);

    // Get next msg_seq for this group
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "SELECT COALESCE(MAX(msg_seq), 0) + 1 FROM group_messages WHERE group_id = ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, group_id);
    sqlite3_step(stmt);
    int64_t next_seq = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);

    // Insert
    sqlite3_prepare_v2(db_,
        "INSERT INTO group_messages (group_id, msg_seq, from_uid, content, msg_type, timestamp) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, group_id);
    sqlite3_bind_int64(stmt, 2, next_seq);
    sqlite3_bind_int64(stmt, 3, from_uid);
    sqlite3_bind_text(stmt, 4, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, msg_type);
    sqlite3_bind_int64(stmt, 6, now);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
        return -1;
    }
    sqlite3_finalize(stmt);
    sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);
    return next_seq;
}

std::vector<GroupMessageRow> Database::getGroupHistory(int64_t group_id,
                                                        int64_t before_msg_seq, int limit)
{
    std::string sql = R"(
        SELECT gm.id, gm.group_id, gm.msg_seq, gm.from_uid, u.nickname,
               gm.content, gm.msg_type, gm.timestamp
        FROM group_messages gm
        JOIN users u ON u.id = gm.from_uid
        WHERE gm.group_id = ?
    )";
    if (before_msg_seq > 0)
        sql += " AND gm.msg_seq < ?";
    sql += " ORDER BY gm.msg_seq DESC LIMIT ?";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

    int idx = 1;
    sqlite3_bind_int64(stmt, idx++, group_id);
    if (before_msg_seq > 0)
        sqlite3_bind_int64(stmt, idx++, before_msg_seq);
    sqlite3_bind_int(stmt, idx++, limit);

    std::vector<GroupMessageRow> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GroupMessageRow r;
        r.id            = sqlite3_column_int64(stmt, 0);
        r.group_id      = sqlite3_column_int64(stmt, 1);
        r.msg_seq       = sqlite3_column_int64(stmt, 2);
        r.from_uid      = sqlite3_column_int64(stmt, 3);
        r.from_nickname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        r.content       = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        r.msg_type      = sqlite3_column_int(stmt, 6);
        r.timestamp     = sqlite3_column_int64(stmt, 7);
        rows.push_back(r);
    }
    sqlite3_finalize(stmt);

    std::reverse(rows.begin(), rows.end());
    return rows;
}

int64_t Database::createGroupApply(int64_t group_id, int64_t from_uid,
                                    const std::string& message)
{
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "INSERT INTO group_requests (group_id, from_uid, message, created_at) VALUES (?, ?, ?, ?)",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, group_id);
    sqlite3_bind_int64(stmt, 2, from_uid);
    sqlite3_bind_text(stmt, 3, message.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, static_cast<int64_t>(std::time(nullptr)));

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db_);
}

bool Database::handleGroupApply(int64_t request_id, const std::string& action)
{
    int newStatus = (action == "accept") ? 1 : 2;
    int64_t now = static_cast<int64_t>(std::time(nullptr));

    // Get request info
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "SELECT group_id, from_uid FROM group_requests WHERE id = ? AND status = 0",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, request_id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false;
    }
    int64_t group_id = sqlite3_column_int64(stmt, 0);
    int64_t from_uid = sqlite3_column_int64(stmt, 1);
    sqlite3_finalize(stmt);

    // Update status
    sqlite3_prepare_v2(db_,
        "UPDATE group_requests SET status = ?, handled_at = ? WHERE id = ?",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, newStatus);
    sqlite3_bind_int64(stmt, 2, now);
    sqlite3_bind_int64(stmt, 3, request_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // If accepted, add member
    if (newStatus == 1) {
        joinGroup(group_id, from_uid);
    }

    return true;
}

int64_t Database::getGroupRequestGroupId(int64_t request_id)
{
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_,
        "SELECT group_id FROM group_requests WHERE id = ? AND status = 0",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, request_id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return -1;
    }
    int64_t group_id = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return group_id;
}
