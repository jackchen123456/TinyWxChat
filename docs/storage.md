# TinyWeChat 数据库设计 v0.1（草案）

> 存储引擎：SQLite 3
> 项目路径：`wx-server/data/tinywechat.db`（服务端启动时自动创建）

---

## 1. 总览

| 表名 | 用途 | 创建阶段 |
|------|------|----------|
| `users` | 用户账号信息 | Phase 0 |
| `messages` | 单聊消息 | Phase 1 |
| `friends` | 好友关系 | Phase 2 |
| `friend_requests` | 好友申请记录 | Phase 2 |
| `groups` | 群聊信息 | Phase 3 |
| `group_members` | 群成员 | Phase 3 |
| `group_messages` | 群聊消息 | Phase 3 |
| `group_requests` | 群聊申请记录 | Phase 3 |

> **为什么单聊和群聊消息分两个表？**
> 消息 ID 分配策略不同：单聊使用全局自增 `msg_id`，群聊使用群内自增 `msg_seq`。分表避免 ID 冲突和索引膨胀。后续若性能需要可合并优化。

---

## 2. 表定义

### 2.1 users

```sql
CREATE TABLE users (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    username    TEXT    NOT NULL UNIQUE,
    password    TEXT    NOT NULL,          -- bcrypt 哈希后存储
    nickname    TEXT    NOT NULL DEFAULT '',
    avatar_path TEXT    DEFAULT NULL,      -- 头像本地路径，Phase 2 启用
    created_at  INTEGER NOT NULL           -- Unix 时间戳（秒）
);

CREATE INDEX idx_users_username ON users(username);
```

- `username`：最长 32 字节（UTF-8），服务端在注册时校验
- `password`：使用 bcrypt 哈希存储，**绝不存明文**
- `avatar_path`：服务端本地路径，如 `/data/avatars/1.jpg`

### 2.2 messages（单聊消息）

```sql
CREATE TABLE messages (
    msg_id      INTEGER PRIMARY KEY AUTOINCREMENT,  -- 全局自增
    from_uid    INTEGER NOT NULL,
    to_uid      INTEGER NOT NULL,
    content     TEXT    NOT NULL,                    -- 文本内容，最大 4096 字节
    msg_type    INTEGER NOT NULL DEFAULT 1,          -- 1=文本, 2=表情(Phase3), 3=图片(Phase3)
    extra       TEXT    DEFAULT NULL,                -- JSON 扩展字段（如图片路径/表情 ID）
    timestamp   INTEGER NOT NULL,                    -- 发送时间（Unix 秒）
    FOREIGN KEY (from_uid) REFERENCES users(id),
    FOREIGN KEY (to_uid)   REFERENCES users(id)
);

CREATE INDEX idx_messages_pair ON messages(from_uid, to_uid);
CREATE INDEX idx_messages_to   ON messages(to_uid, timestamp DESC);
```

> **索引说明**：`idx_messages_pair(from_uid, to_uid)` 配合 SQLite 的自动索引翻转，对单边 `from=? AND to=?` 查询覆盖良好。`(from=A AND to=B) OR (from=B AND to=A)` 的 OR 双支查询在小数据量下性能可接受；若后续数据增长，可增加 `idx_messages_rev(to_uid, from_uid)`。

**历史拉取查询（按会话）：**
```sql
-- 获取 A 和 B 之间的最近 20 条消息
SELECT * FROM messages
WHERE (from_uid = A AND to_uid = B) OR (from_uid = B AND to_uid = A)
ORDER BY msg_id DESC
LIMIT 20;

-- 分页（before_msg_id = 50）
SELECT * FROM messages
WHERE ((from_uid = A AND to_uid = B) OR (from_uid = B AND to_uid = A))
  AND msg_id < 50
ORDER BY msg_id DESC
LIMIT 20;
```

### 2.3 friends

```sql
CREATE TABLE friends (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    user_a      INTEGER NOT NULL,
    user_b      INTEGER NOT NULL,
    created_at  INTEGER NOT NULL,
    FOREIGN KEY (user_a) REFERENCES users(id),
    FOREIGN KEY (user_b) REFERENCES users(id)
);

-- 约束：避免重复好友关系（小号在前）
CREATE UNIQUE INDEX idx_friends_pair
    ON friends( MIN(user_a, user_b), MAX(user_a, user_b) );
```

- 存储时保证 `user_a < user_b`，避免 (1,2) 和 (2,1) 重复

### 2.4 friend_requests

```sql
CREATE TABLE friend_requests (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    from_uid    INTEGER NOT NULL,
    to_uid      INTEGER NOT NULL,
    message     TEXT    DEFAULT '',        -- 申请附言
    status      INTEGER NOT NULL DEFAULT 0, -- 0=待处理, 1=已同意, 2=已拒绝
    created_at  INTEGER NOT NULL,
    handled_at  INTEGER DEFAULT NULL,      -- 处理时间
    FOREIGN KEY (from_uid) REFERENCES users(id),
    FOREIGN KEY (to_uid)   REFERENCES users(id)
);

CREATE INDEX idx_fr_to ON friend_requests(to_uid, status);
```

### 2.5 groups

```sql
CREATE TABLE groups (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT    NOT NULL,
    owner_uid   INTEGER NOT NULL,          -- 群主
    avatar_path TEXT    DEFAULT NULL,
    created_at  INTEGER NOT NULL,
    FOREIGN KEY (owner_uid) REFERENCES users(id)
);
```

### 2.6 group_members

```sql
CREATE TABLE group_members (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    group_id    INTEGER NOT NULL,
    user_id     INTEGER NOT NULL,
    role        INTEGER NOT NULL DEFAULT 0, -- 0=普通成员, 1=管理员, 2=群主
    joined_at   INTEGER NOT NULL,
    FOREIGN KEY (group_id) REFERENCES groups(id),
    FOREIGN KEY (user_id)  REFERENCES users(id),
    UNIQUE(group_id, user_id)
);

CREATE INDEX idx_gm_group ON group_members(group_id);
CREATE INDEX idx_gm_user  ON group_members(user_id);
```

- 群主既是 `groups.owner_uid` 也在 `group_members` 中有条目（role=2）

### 2.7 group_messages

```sql
CREATE TABLE group_messages (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,  -- 行 ID（内部使用）
    group_id    INTEGER NOT NULL,
    msg_seq     INTEGER NOT NULL,                    -- 群内自增序列号（用于拉取）
    from_uid    INTEGER NOT NULL,
    content     TEXT    NOT NULL,
    msg_type    INTEGER NOT NULL DEFAULT 1,          -- 同 messages.msg_type
    extra       TEXT    DEFAULT NULL,
    timestamp   INTEGER NOT NULL,
    FOREIGN KEY (group_id) REFERENCES groups(id),
    FOREIGN KEY (from_uid)  REFERENCES users(id)
);

-- 群内唯一序列号
CREATE UNIQUE INDEX idx_gm_seq ON group_messages(group_id, msg_seq);
CREATE INDEX idx_gm_group_ts  ON group_messages(group_id, msg_seq DESC);
```

**说明：**
- `msg_seq` 在同一 `group_id` 内自增，不是全局自增
- 实现方式：`INSERT ... SELECT COALESCE(MAX(msg_seq), 0) + 1 FROM group_messages WHERE group_id = ?`（在事务中执行）

**历史拉取：**
```sql
-- 获取群 1 的最新的 20 条消息
SELECT * FROM group_messages
WHERE group_id = 1
ORDER BY msg_seq DESC
LIMIT 20;

-- 分页（before_msg_seq = 100）
SELECT * FROM group_messages
WHERE group_id = 1 AND msg_seq < 100
ORDER BY msg_seq DESC
LIMIT 20;
```

### 2.8 group_requests

```sql
CREATE TABLE group_requests (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    group_id    INTEGER NOT NULL,
    from_uid    INTEGER NOT NULL,
    message     TEXT    DEFAULT '',
    status      INTEGER NOT NULL DEFAULT 0, -- 0=待处理, 1=已同意, 2=已拒绝
    created_at  INTEGER NOT NULL,
    handled_at  INTEGER DEFAULT NULL,
    FOREIGN KEY (group_id) REFERENCES groups(id),
    FOREIGN KEY (from_uid) REFERENCES users(id)
);

CREATE INDEX idx_gr_group ON group_requests(group_id, status);
```

---

## 3. 索引策略总结

| 表 | 索引 | 用途 |
|----|------|------|
| users | `idx_users_username` | 登录查询 |
| messages | `idx_messages_pair` | 会话历史拉取 |
| messages | `idx_messages_to` | 按目标用户+时间排序（未使用） |
| friends | `idx_friends_pair` | 好友关系存在性检查 |
| friend_requests | `idx_fr_to` | 用户查询待处理申请 |
| group_members | `idx_gm_group` | 群成员列表 |
| group_members | `idx_gm_user` | 用户参与哪些群 |
| group_messages | `idx_gm_seq` | 群消息拉取 |
| group_requests | `idx_gr_group` | 群申请列表 |

---

## 4. 数据清理策略

MVP 暂不做自动清理。后续可考虑：
- 单聊消息仅保留最近 N 天（如 90 天）
- 已拒绝的好友申请保留 30 天后自动删除

---

## 4.5 会话列表查询（从 messages 聚合）

> 设计决策：不建独立 conversations 表，直接从 messages 聚合最近会话（见 `requirements.md` §0.1 决策 #5）。

**查询最近会话（单聊）：**

```sql
-- 获取 user_id=1 的最近 50 个会话
SELECT
  CASE WHEN from_uid = 1 THEN to_uid ELSE from_uid END AS peer_id,
  MAX(timestamp) AS last_time,
  (SELECT content FROM messages
   WHERE msg_id = (
     SELECT MAX(msg_id) FROM messages
     WHERE (from_uid = 1 AND to_uid = peer_id)
        OR (from_uid = peer_id AND to_uid = 1)
   )) AS last_content
FROM messages
WHERE from_uid = 1 OR to_uid = 1
GROUP BY peer_id
ORDER BY last_time DESC
LIMIT 50;
```

**注意事项：**
- 此查询在消息量 < 10 万条时性能可接受
- 后续数据增长可增加 `conversations` 独立表做写时维护
- MVP 阶段不做未读计数（需要额外的 `last_read_msg_id` 字段）

---

## 5. 注意

- 所有时间戳为 Unix 秒（`INTEGER`），时区统一为 UTC，客户端展示时做本地转换
- 密码使用 bcrypt，服务端依赖 `libbcrypt` 或系统 `crypt()`
- SQLite 使用 WAL 模式提升并发读性能：
  ```sql
  PRAGMA journal_mode=WAL;
  ```
