# TinyWeChat 端到端时序 v0.1（草案）

> 时序图采用从上到下的时间轴，箭头上的标注对应协议消息 type。

---

## 1. 注册流程

```
Client A                        Server                    SQLite
   │                               │                         │
   │──── TCP connect ──────────────>│                         │
   │<─── TCP accept ───────────────│                         │
   │                               │                         │
   │──── auth.register ───────────>│                         │
   │    {username, password,       │                         │
   │     nickname}                 │                         │
   │                               │──── 校验 username       │
   │                               │    (非空、≤32B、不含空格)│
   │                               │──── 校验 password       │
   │                               │    (≥6B、≤128B)         │
   │                               │──── bcrypt(password) ──>│
   │                               │──── INSERT users ──────>│
   │                               │<─── OK ────────────────│
   │<─── auth.register_res ───────│                         │
   │    {code:0, user_id:1}       │                         │
   │                               │                         │
```

## 2. 登录 & 心跳 & 在线推送

```
Client A                        Server                    SQLite
   │                               │                         │
   │──── TCP connect ──────────────>│                         │
   │<─── TCP accept ───────────────│                         │
   │                               │                         │
   │──── auth.login ──────────────>│                         │
   │    {username, password}       │                         │
   │                               │──── SELECT users ──────>│
   │                               │       WHERE username    │
   │                               │<─── user row ──────────│
   │                               │──── bcrypt_verify ─────│
   │                               │ (若失败 → 返回 code:401)│
   │                               │                         │
   │                               │──── 检查该 user_id 是否  │
   │                               │     已有活跃 session    │
   │                               │ (有则清理旧 session)    │
   │                               │                         │
   │<─── auth.login_res ──────────│                         │
   │    {code:0, user_id,         │                         │
   │     nickname}                │                         │
   │                               │                         │
   │══════════════ 保持在线 ════════│                         │
   │                               │                         │
   │──── (每30s) ping ────────────>│                         │
   │<─── pong ─────────────────────│                         │
   │                               │                         │
   │══════════════ 断线场景 ════════│                         │
   │(网络断开 / 客户端退出)          │                         │
   │                               │── 90s 无数据，清理 ────│
   │                               │ session 和 user 映射    │
```

同一用户重复登录（例如另一处登录）：服务端断开旧连接，旧客户端在下一次发消息/心跳时发现连接已关闭。

## 3. 单聊文本消息（双方在线）

```
Client A                        Server                    Client B
   │                               │                         │
   │──── chat.send ───────────────>│                         │
   │    {to_user_id:2,            │                         │
   │     content:"你好"}          │                         │
   │                               │                         │
   │                               │──── 校验 A 是否已登录    │
   │                               │    (无 session → code:210)│
   │                               │──── 校验 to_user 存在    │
   │                               │──── 校验 A 和 B 是好友    │
   │                               │    (Phase 2+, Phase 1 跳过)│
   │                               │                         │
   │                               │──── INSERT messages ────│
   │                               │    (from_uid=1,         │
   │                               │     to_uid=2, ...)      │
   │                               │                         │
   │<─── chat.send_res ───────────│                         │
   │    {code:0, msg_id:12345,   │                         │
   │     timestamp:1700000000}    │                         │
   │                               │                         │
   │                               │── 查找 user_id=2 的     │
   │                               │   session（在线判断）    │
   │                               │                         │
   │                               │──── chat.recv ──────────>│
   │                               │    {from_user_id:1,     │
   │                               │     from_nickname:"Alice",│
   │                               │     msg_id:12345,       │
   │                               │     content:"你好",     │
   │                               │     timestamp:...}      │
   │                               │                         │
```

## 4. 单聊离线消息（B 不在线）

```
Client A                        Server                    SQLite
   │                               │                         │
   │──── chat.send ───────────────>│                         │
   │    {to_user_id:2,            │                         │
   │     content:"明天见"}         │                         │
   │                               │                         │
   │                               │──── INSERT messages ────│
   │                               │    (同上，正常落库)      │
   │                               │                         │
   │                               │──── 查找 user_id=2 的   │
   │                               │    session → 未找到     │
   │                               │    → 不推送，不报错     │
   │                               │    → 消息已落库         │
   │                               │    下次登录拉历史可见    │
   │                               │                         │
   │<─── chat.send_res ───────────│                         │
   │    {code:0, msg_id:12346,   │                         │
   │     timestamp:...}           │                         │
   │                               │                         │
```

## 5. 历史消息拉取

```
Client B (刚登录)                 Server                    SQLite
   │                               │                         │
   │──── chat.history ────────────>│                         │
   │    {with_user_id:1,          │                         │
   │     before_msg_id:0,         │                         │
   │     limit:20}                │                         │
   │                               │                         │
   │                               │──── SELECT * FROM msgs ─>│
   │                               │    WHERE (from=1 AND    │
   │                               │           to=2)         │
   │                               │       OR (from=2 AND    │
   │                               │           to=1)         │
   │                               │    ORDER BY msg_id DESC │
   │                               │    LIMIT 20             │
   │                               │<─── messages[] ────────│
   │                               │                         │
   │<─── chat.history_res ────────│                         │
   │    {code:0,                  │                         │
   │     messages: [{msg_id,      │                         │
   │       from_user_id,          │                         │
   │       content, timestamp},   │                         │
   │       ...]                   │                         │
   │                               │                         │
```

## 6. 好友申请流程

```
Client A                        Server                    Client B
   │                               │                         │
   │──── friend.request ──────────>│                         │
   │    {to_user_id:2,            │                         │
   │     message:"加个好友"}       │                         │
   │                               │                         │
   │                               │──── 校验 B 存在          │
   │                               │──── 校验非已经是好友      │
   │                               │──── 校验无重复待处理申请  │
   │                               │                         │
   │                               │──── INSERT friend_req ──│
   │                               │                         │
   │<─── friend.request_res ──────│                         │
   │    {code:0}                  │                         │
   │                               │                         │
   │                               │── 查找 B 的 session      │
   │                               │                         │
   │                               │──── friend.notify ──────>│
   │                               │    {request_id:1,       │
   │                               │     from_user_id:1,     │
   │                               │     from_nickname:"Alice",│
   │                               │     message:"加个好友"}  │
   │                               │                         │
   │══════════ B 处理申请 ══════════│                         │
   │                               │                         │
   │                               │<──── friend.handle ─────│
   │                               │    {request_id:1,       │
   │                               │     action:"accept"}    │
   │                               │                         │
   │                               │──── UPDATE status=1     │
   │                               │──── INSERT friends      │
   │                               │    (user_a=1, user_b=2) │
   │                               │                         │
   │                               │────> friend.handle_res  │
   │                               │    {code:0}             │
   │                               │                         │
   │                               │── (可选通知 A：已通过)   │
```

## 7. 群聊消息广播

```
Client A (群1 成员)             Server                群其他成员
   │                               │                         │
   │──── group.send ──────────────>│                         │
   │    {group_id:1,              │                         │
   │     content:"大家好"}         │                         │
   │                               │                         │
   │                               │──── 校验 A 是群成员      │
   │                               │──── INSERT group_msgs   │
   │                               │    (group_id=1,         │
   │                               │     msg_seq=MAX+1)      │
   │                               │                         │
   │<─── group.send_res ──────────│                         │
   │    {code:0, msg_seq:5,      │                         │
   │     timestamp:...}           │                         │
   │                               │                         │
   │                               │──── 遍历群成员 B,C,...   │
   │                               │    (跳过 A 自己)         │
   │                               │    → 查找每个成员 session│
   │                               │                         │
   │                               │──── group.recv ─────────>│
   │                               │    → 推送给每个在线成员  │
   │                               │    {group_id:1,         │
   │                               │     from_user_id:1,     │
   │                               │     from_nickname:"Alice",│
   │                               │     msg_seq:5,          │
   │                               │     content:"大家好",    │
   │                               │     timestamp:...}      │
   │                               │                         │
   │                               │── 离线成员：不推送，     │
   │                               │   下次拉历史可见         │
```

## 8. 创建群聊

```
Client A                        Server                    SQLite
   │                               │                         │
   │──── group.create ────────────>│                         │
   │    {name:"项目群"}            │                         │
   │                               │                         │
   │                               │──── INSERT groups ──────>│
   │                               │    (name, owner_uid=A)  │
   │                               │<─── group_id:1 ────────│
   │                               │                         │
   │                               │──── INSERT group_members│
   │                               │    (group_id=1,         │
   │                               │     user_id=A, role=2)  │
   │                               │                         │
   │<─── group.create_res ────────│                         │
   │    {code:0, group_id:1}      │                         │
```

---

## 9. 错误路径汇总

### 9.1 Token/登录校验失败（任何需要鉴权的请求）

```
Client                        Server
  │                              │
  │──── 任意消息 ───────────────>│
  │                              │── 校验 session → 无
  │<─── error通知 ──────────────│
  │    {code:210,               │
  │     message:"未登录"}        │
```

### 9.2 超时断开

```
Client                        Server
  │                              │
  │  (无任何数据 90 秒)          │
  │                              │── close() 连接
  │<── TCP FIN ────────────────│
```

### 9.3 帧格式错误

```
Client                        Server
  │                              │
  │──── 非法帧 ─────────────────>│
  │    (length>65535 / 不识别    │
  │     的 frame_type)           │
  │                              │
  │<─── close() 连接 ───────────│
  │    (不做 error 回复 ——       │
  │     帧头错误说明无法解析)     │
```

---

## 10. 服务端连接生命周期状态机

```
                     ┌─────────────┐
                     │  CONNECTED  │  ← TCP 连接建立
                     └──────┬──────┘
                            │ auth.login 成功
                     ┌──────v──────┐
               ┌─────┤  LOGGED_IN  ├─────┐
               │     └──────┬──────┘     │
               │            │            │
     chat.send  │    group.send   friend.request
               │            │            │
               v            v            v
         (正常处理)    (正常处理)    (正常处理)
               
               超时 90s / 客户端断开 / 重复登录踢下线
                            │
                     ┌──────v──────┐
                     │  DISCONNECT │  → 清理 session
                     └─────────────┘
```

服务端维护两个核心映射：
1. `session_id → ConnectionState`（连接状态）
2. `user_id → session_id`（登录后建立，下线删除）
