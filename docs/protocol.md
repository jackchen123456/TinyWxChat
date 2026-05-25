# TinyWeChat 协议 v0.1（草案）

> 协议基础：TCP + 二进制帧头 + JSON Payload
> 后续可升级为 protobuf，帧头兼容

> 📌 **协议常量真相源**
> C++ 两端的 `MsgType` / `ErrCode` / `FrameType` / 帧大小常量都在 [protocol/common.h](../protocol/common.h)，由 wx-client 与 wx-server 共同 include。
> 改名/新增常量 = 一次编辑、双端编译保证一致；不存在镜像漂移问题。
>
> 仍需手动同步的：
> - 本文档（值/语义的人类描述）
> - Python 测试脚本（`wx-server/test_*.py` 里硬编码的 type 字符串）
>

---

## 1. 帧格式（Wire Format）

每一帧（Frame）由 **定长头部** + **可变长 Payload** 组成。

### 1.1 二进制帧头（8 字节）

```
Offset  Size  Field        Description
────────────────────────────────────────
0       4     length       Payload 长度（大端 uint32），不含帧头本身
4       2     version      协议版本号，当前固定 0x0001
6       1     frame_type   帧类型（见下文）
7       1     flags        标志位（预留，当前填 0x00）
```

- `length` 最大 **1 MiB（1048576 字节）**，超出时客户端/服务端应当断开连接。这个上限为了承载 base64 编码后的图片（msg_type=3）。
- `version` 目前仅 `0x0001`
- `frame_type`：
  - `0x01` = REQUEST（请求）
  - `0x02` = RESPONSE（响应）
  - `0x03` = NOTIFICATION（通知，无需响应）
  - `0x04` = PING / PONG（心跳）
- `flags`：bit 0 设 1 表示 Payload 为 gzip 压缩（MVP 暂不使用）

### 1.2 Payload（JSON）

Payload 是 UTF-8 编码的 JSON 对象，长度不超过 1 MiB（见 §1.1）。

通用结构：

```json
{
  "type": "<消息类型字符串>",
  "seq": 1,
  "body": { ... }
}
```

- `seq`：请求端的消息序号（递增），用于对应请求和响应。服务端在响应中携带相同的 `seq`。
- `type`：见 §2 消息类型定义
- `body`：具体消息内容

---

## 2. 消息类型（Message Types）

### 2.1 基础

| type | direction | 说明 |
|------|-----------|------|
| `auth.login` | C→S | 登录 |
| `auth.login_res` | S→C | 登录结果 |
| `auth.register` | C→S | 注册 |
| `auth.register_res` | S→C | 注册结果 |

### 2.2 单聊

| type | direction | 说明 |
|------|-----------|------|
| `chat.send` | C→S | 发送单聊消息 |
| `chat.send_res` | S→C | 发送结果 |
| `chat.recv` | S→C | 服务端推送收到的消息（通知类型） |
| `chat.history` | C→S | 拉取历史消息 |
| `chat.history_res` | S→C | 历史消息列表 |
| `chat.conversations` | C→S | 拉取会话列表（从 messages 聚合） |
| `chat.conversations_res` | S→C | 会话列表 |

### 2.3 群聊

| type | direction | 说明 |
|------|-----------|------|
| `group.create` | C→S | 创建群聊 |
| `group.create_res` | S→C | 创建结果 |
| `group.join` | C→S | 加入群聊 |
| `group.join_res` | S→C | 加入结果 |
| `group.send` | C→S | 发群消息 |
| `group.send_res` | S→C | 发送结果 |
| `group.recv` | S→C | 群消息推送 |
| `group.history` | C→S | 拉取群历史 |
| `group.history_res` | S→C | 群历史列表 |
| `group.list` | C→S | 拉取用户加入的群列表 |
| `group.list_res` | S→C | 群列表 |

### 2.4 好友系统

| type | direction | 说明 |
|------|-----------|------|
| `friend.request` | C→S | 发起好友申请 |
| `friend.request_res` | S→C | 申请结果 |
| `friend.notify` | S→C | 通知被申请人有新的好友申请 |
| `friend.handle` | C→S | 处理好友申请（同意/拒绝） |
| `friend.handle_res` | S→C | 处理结果 |
| `friend.list` | C→S | 拉取好友列表 |
| `friend.list_res` | S→C | 好友列表 |
| `friend.pending` | C→S | 拉取待处理的申请列表 |
| `friend.pending_res` | S→C | 待处理列表 |

### 2.5 群申请（Phase 3）

| type | direction | 说明 |
|------|-----------|------|
| `group.apply` | C→S | 申请加入群聊 |
| `group.apply_res` | S→C | 申请结果 |
| `group.apply_notify` | S→C | 通知群主/管理员有新申请 |
| `group.apply_handle` | C→S | 处理群申请 |
| `group.apply_handle_res` | S→C | 处理结果 |

### 2.6 通用 & 心跳

| type | direction | 说明 |
|------|-----------|------|
| `ping` | C→S | 心跳请求 |
| `pong` | S→C | 心跳回复 |
| `error` | S→C | 通用错误。若原请求带有 seq，走 RESPONSE（`frame_type=0x02`）便于客户端按 seq 关联回原请求；无 seq 时走 NOTIFICATION（`frame_type=0x03`）。 |

---

## 3. 消息体（Body）字段定义

### auth.login

```
C→S:  { "username": "alice", "password": "xxx" }
S→C:  { "code": 0, "user_id": 1, "nickname": "Alice" }
```

### auth.register

```
C→S:  { "username": "alice", "password": "xxx", "nickname": "Alice" }
S→C:  { "code": 0, "user_id": 1 }
```

### chat.send

```
C→S:  { "to_user_id": 2, "content": "你好", "msg_type": 1 }
S→C:  { "code": 0, "msg_id": 12345, "timestamp": 1700000000 }
```

- `msg_type`：1=文本, 2=表情, 3=图片
- `content` 长度上限（服务端校验，超长返回 `code=500 MESSAGE_TOO_LONG`）：
  - msg_type=1（文本）/ msg_type=2（表情）：**4096 字节**
  - msg_type=3（图片）：**800 KiB**（base64 后的字符串长度；原图建议 ≤ 500 KB）
- 图片消息约定：客户端将图片读取后 `QByteArray::toBase64()` 放进 `content`；`msg_type=3`；接收端用 `QByteArray::fromBase64()` 还原。无单独的二进制传输通道。

### chat.recv（服务端推送）

```
{ "from_user_id": 1, "from_nickname": "Alice",
  "msg_id": 12345, "content": "你好", "msg_type": 1, "timestamp": 1700000000 }
```

### chat.history

```
C→S:  { "with_user_id": 2, "before_msg_id": 0, "limit": 20 }
S→C:  { "code": 0, "messages": [ ... ] }
```

- `before_msg_id=0` 表示从最新开始拉取；非 0 表示拉取早于此 ID 的消息。

### chat.conversations

```
C→S:  {}
S→C:  { "code": 0, "conversations": [
        { "peer_id": 2, "peer_nickname": "Bob",
          "last_content": "你好", "last_timestamp": 1700000000 }, ... ] }
```

- 从 messages 表聚合，按 last_timestamp 降序，最多 50 条。

### group.create

```
C→S:  { "name": "项目群" }
S→C:  { "code": 0, "group_id": 1 }
```

### group.join

```
C→S:  { "group_id": 1 }
S→C:  { "code": 0 }
```

### group.send

```
C→S:  { "group_id": 1, "content": "大家好", "msg_type": 1 }
S→C:  { "code": 0, "msg_seq": 5, "timestamp": 1700000000 }
```

- `msg_type` 与 content 长度规则同 `chat.send`（文本/表情 4096 字节、图片 800 KiB base64）。
- 非群成员发送 → `code=322 GROUP_PERMISSION_DENIED`。

### group.recv（服务端推送）

```
{ "group_id": 1, "from_user_id": 1, "from_nickname": "Alice",
  "msg_seq": 5, "content": "大家好", "msg_type": 1, "timestamp": 1700000000 }
```

### group.history

```
C→S:  { "group_id": 1, "before_msg_seq": 0, "limit": 20 }
S→C:  { "code": 0, "messages": [ ... ] }
```

- `before_msg_seq=0` 表示从最新开始拉取；非 0 表示拉取早于此 seq 的消息。

### group.list

```
C→S:  {}
S→C:  { "code": 0, "groups": [
        { "group_id": 1, "group_name": "项目群" }, ... ] }
```

- 返回当前用户加入的所有群列表。

### friend.request

```
C→S:  { "to_user_id": 2, "message": "加个好友" }
S→C:  { "code": 0 }
```

### friend.notify

```
{ "request_id": 1, "from_user_id": 1, "from_nickname": "Alice",
  "message": "加个好友", "timestamp": 1700000000 }
```

### friend.handle

```
C→S:  { "request_id": 1, "action": "accept" }  // 或 "reject"
S→C:  { "code": 0 }
```

### friend.list

```
C→S:  {}
S→C:  { "code": 0, "friends": [{"user_id": 2, "nickname": "Bob"}, ...] }
```

### friend.pending

```
C→S:  {}
S→C:  { "code": 0, "requests": [{"request_id": 1, "from_user_id": 1, "from_nickname": "Alice", "message": "...", "timestamp": ...}, ...] }
```

---

## 4. 错误码（Error Codes）

所有响应和 `error` 通知都通过 `code` 字段传递状态。

| code | 含义 | 说明 |
|------|------|------|
| 0 | SUCCESS | 成功 |
| 100 | UNKNOWN_ERROR | 未知错误 |
| 101 | INTERNAL_ERROR | 服务端内部错误 |
| 200 | INVALID_REQUEST | 请求格式错误/字段缺失 |
| 201 | INVALID_FRAME | 帧格式错误（长度超限/版本不支持） |
| 210 | UNAUTHORIZED | 未登录或 token 无效 |
| 211 | SESSION_EXPIRED | 会话过期（暂不实现） |
| 300 | USER_NOT_FOUND | 目标用户不存在 |
| 301 | USER_OFFLINE | 目标用户不在线 |
| 310 | FRIEND_NOT_FOUND | 不是好友关系 |
| 311 | FRIEND_ALREADY | 已经是好友 |
| 312 | FRIEND_REQUEST_EXISTS | 好友申请已存在（待处理） |
| 320 | GROUP_NOT_FOUND | 群不存在 |
| 321 | GROUP_ALREADY_MEMBER | 已是群成员 |
| 322 | GROUP_PERMISSION_DENIED | 无权限（非群主/管理员处理申请） |
| 400 | DUPLICATE_USERNAME | 用户名已被注册 |
| 401 | WRONG_PASSWORD | 密码错误 |
| 402 | USERNAME_TOO_LONG | 用户名超长（最大32字节） |
| 500 | MESSAGE_TOO_LONG | 消息内容超长（文本最大4096字节） |
| 501 | RATE_LIMITED | 发送频率过快 |

错误通知（`type: "error"`）格式：

```json
{
  "code": 210,
  "message": "未登录，请先登录"
}
```

---

## 5. 心跳

- 客户端每隔 **30 秒**发送一个 PING 帧（frame_type=0x04，Payload 为空或 `{}`）
- 服务端回复 PONG 帧（frame_type=0x04，Payload 为空或 `{}`）
- 服务端 **90 秒**内未收到任何数据（含 PING）则断开连接
- 客户端 **90 秒**内未收到 PONG 或任何数据则主动断开并尝试重连

---

## 6. 连接生命周期

```
[CLIENT]                          [SERVER]
   │                                │
   │──── TCP connect ──────────────>│
   │<─── TCP accept + session ─────│
   │                                │
   │──── auth.login ───────────────>│
   │<─── auth.login_res (code=0) ──│  ← 服务端记录 user→session 映射
   │                                │
   │──── 正常消息交互 ──────────────│
   │──── (每 30s) ping ───────────>│
   │<─── pong ─────────────────────│
   │                                │
   │──── 断开 / 超时 90s ──────────│
   │                                │  ← 服务端清理 session 映射
```

同一用户不允许同时建立多个登录连接：新登录会踢掉旧连接（服务端在 login 成功时主动断开该 user_id 的旧连接）。客户端若被踢，将在下一次 PING 或收发操作时发现连接已关闭。

---

## 7. 默认值 & 约束汇总

| 项 | 值 |
|----|----|
| 最大 Payload 长度 | 1 MiB（1048576 字节） |
| 最大文本消息长度（msg_type=1/2） | 4096 字节 |
| 最大图片消息长度（msg_type=3） | 800 KiB（base64 后） |
| 单个会话写缓冲上限（双向） | 4 MiB |
| 心跳间隔（客户端） | 30 秒 |
| 连接超时（服务端） | 90 秒无数据 |
| 历史拉取默认条数 | 20 条 |
| 协议版本 | `0x0001` |
| 用户名最大长度 | 32 字节（UTF-8） |
| 密码最小长度 | 6 字节 |
| 服务端最大并发连接 | 256 |
