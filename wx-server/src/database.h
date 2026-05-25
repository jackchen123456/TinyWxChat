#pragma once

#include <string>
#include <vector>
#include <optional>
#include <mutex>
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
    int msg_type = 1;
    int64_t timestamp = 0;
};

// ── Phase 2: Friend ──────────────────────────────────────
struct FriendRequestRow {
    int64_t id = 0;
    int64_t from_uid = 0;
    std::string from_nickname;
    int64_t to_uid = 0;
    std::string message;
    int status = 0;       // 0=pending, 1=accepted, 2=rejected
    int64_t created_at = 0;
};

struct FriendRow {
    int64_t user_id = 0;
    std::string nickname;
};

// ── Phase 3: Group ──────────────────────────────────────
struct GroupMessageRow {
    int64_t id = 0;
    int64_t group_id = 0;
    int64_t msg_seq = 0;
    int64_t from_uid = 0;
    std::string from_nickname;
    std::string content;
    int msg_type = 1;
    int64_t timestamp = 0;
};

struct GroupMemberRow {
    int64_t user_id = 0;
    int role = 0;        // 0=member, 1=admin, 2=owner
};

struct GroupRequestRow {
    int64_t id = 0;
    int64_t group_id = 0;
    int64_t from_uid = 0;
    std::string from_nickname;
    std::string message;
    int status = 0;
    int64_t created_at = 0;
};

class Database {
public:
    ~Database();

    bool open(const std::string& path);

    /// Direct access to raw sqlite3 handle (for ad-hoc queries).
    sqlite3* rawDB() { return db_; }

    // ── Auth ─────────────────────────────────────────────
    std::optional<UserInfo> authenticate(const std::string& username,
                                         const std::string& password);
    int64_t registerUser(const std::string& username,
                         const std::string& password,
                         const std::string& nickname);
    std::optional<UserInfo> userById(int64_t uid);

    // ── Messages (Phase 1) ───────────────────────────────
    int64_t saveMessage(int64_t from_uid, int64_t to_uid,
                        const std::string& content, int msg_type = 1);
    std::vector<MessageRow> getHistory(int64_t uid_a, int64_t uid_b,
                                       int64_t before_msg_id, int limit);

    /// Returns the user's recent conversations aggregated from messages.
    struct ConversationRow {
        int64_t peer_id = 0;
        std::string peer_nickname;
        std::string last_content;
        int64_t last_timestamp = 0;
    };
    std::vector<ConversationRow> getConversations(int64_t uid);

    // ── Friends (Phase 2) ────────────────────────────────
    int64_t createFriendRequest(int64_t from_uid, int64_t to_uid,
                                const std::string& message);
    bool handleFriendRequest(int64_t request_id, const std::string& action);
    std::vector<FriendRow> getFriendList(int64_t uid);
    std::vector<FriendRequestRow> getPendingRequests(int64_t uid);

    // ── Groups (Phase 3) ─────────────────────────────────
    int64_t createGroup(int64_t owner_uid, const std::string& name);
    bool joinGroup(int64_t group_id, int64_t user_id);
    std::vector<GroupMemberRow> getGroupMembers(int64_t group_id);
    bool isGroupMember(int64_t group_id, int64_t user_id);

    /// Get the group owner's uid, or -1 if group doesn't exist.
    int64_t getGroupOwner(int64_t group_id);

    /// Returns groups the user belongs to: {group_id, group_name}
    std::vector<std::pair<int64_t, std::string>> getUserGroups(int64_t user_id);
    int64_t sendGroupMessage(int64_t group_id, int64_t from_uid,
                             const std::string& content, int msg_type);
    std::vector<GroupMessageRow> getGroupHistory(int64_t group_id,
                                                  int64_t before_msg_seq, int limit);
    int64_t createGroupApply(int64_t group_id, int64_t from_uid,
                             const std::string& message);
    bool handleGroupApply(int64_t request_id, const std::string& action);

    /// Get the group_id for a pending group request, or -1 if not found.
    int64_t getGroupRequestGroupId(int64_t request_id);

private:
    void createTables();
    void seedTestUsers();

    // 全局互斥：所有公开方法在内部 lock，确保对 sqlite3* 句柄和共享
    // 预编译语句（stmt_auth_/stmt_insert_msg_）的 reset→bind→step→column
    // 序列是原子的。SQLite 自身的 serialized 模式只保证 sqlite3_step 串行，
    // 不会保护 stmt 对象状态。
    // 用 recursive_mutex 是因为 handleGroupApply 会回调 joinGroup。
    mutable std::recursive_mutex mutex_;

    sqlite3* db_ = nullptr;

    sqlite3_stmt* stmt_auth_       = nullptr;
    sqlite3_stmt* stmt_insert_msg_ = nullptr;
    sqlite3_stmt* stmt_max_id_     = nullptr;
};
