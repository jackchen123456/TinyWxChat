#include "database.h"
#include <iostream>
#include <crypt.h>     // crypt_r
#include <cstring>
#include <ctime>

// ── bcrypt helpers ──────────────────────────────────────

static std::string bcryptHash(const std::string& password)
{
    // $2b$ means bcrypt; 12 rounds is a reasonable default
    // The salt must be exactly 22 chars from [./A-Za-z0-9]
    // We use a deterministic salt here for testing; production would use random.
    static const char* salt_prefix = "$2b$12$abcdefghijklmnopqrstuv";
    struct crypt_data data{};
    char* result = crypt_r(password.c_str(), salt_prefix, &data);
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
    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[DB] Failed to open: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    // Enable WAL mode for better concurrent reads
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;",   nullptr, nullptr, nullptr);

    createTables();
    seedTestUsers();

    // Compile prepared statements
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

    char* err = nullptr;
    sqlite3_exec(db_, users_sql, nullptr, nullptr, &err);
    if (err) { std::cerr << "[DB] users table: " << err << std::endl; sqlite3_free(err); }

    sqlite3_exec(db_, msgs_sql, nullptr, nullptr, &err);
    if (err) { std::cerr << "[DB] messages table: " << err << std::endl; sqlite3_free(err); }
}

void Database::seedTestUsers()
{
    // Check if users already exist
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM users", -1, &stmt, nullptr);
    sqlite3_step(stmt);
    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (count > 0) return;  // already seeded

    std::cout << "[DB] Seeding test users alice / bob (password: 123456) ..." << std::endl;

    int64_t now = static_cast<int64_t>(std::time(nullptr));

    // alice
    registerUser("alice", "123456", "Alice");

    // bob
    registerUser("bob", "123456", "Bob");
}

// ── Auth ────────────────────────────────────────────────

std::optional<UserInfo> Database::authenticate(const std::string& username,
                                                const std::string& password)
{
    sqlite3_reset(stmt_auth_);
    sqlite3_clear_bindings(stmt_auth_);
    sqlite3_bind_text(stmt_auth_, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt_auth_) != SQLITE_ROW)
        return std::nullopt;   // not found

    UserInfo u;
    u.id       = sqlite3_column_int64(stmt_auth_, 0);
    u.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt_auth_, 1));
    u.nickname = reinterpret_cast<const char*>(sqlite3_column_text(stmt_auth_, 2));
    const char* hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt_auth_, 3));

    if (!bcryptVerify(password, hash))
        return std::nullopt;   // wrong password

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

    if (before_msg_id > 0) {
        sql += " AND msg_id < ?";
    }
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

    // Reverse to chronological order
    std::reverse(rows.begin(), rows.end());
    return rows;
}

int64_t Database::maxMsgId()
{
    sqlite3_reset(stmt_max_id_);
    if (sqlite3_step(stmt_max_id_) == SQLITE_ROW)
        return sqlite3_column_int64(stmt_max_id_, 0);
    return 0;
}
