# TinyWeChat

一个“仿微信”的练手项目：**Qt6 客户端** + **Linux C++ 服务端**。

定位：面向 C++/Qt 新手，强调“先跑通、可验收、少返工”。仓库内文档作为单一真相源，后续代码实现全部围绕验收标准推进。

如果你是初学者：把 Hermes 的“多会话 + worktree”当成一个 AI 团队（架构/后端/客户端/联调各一位专家），按本文一步一步推进即可。

> ⚠️ **安全说明（务必先读）**
> 本项目仅用于本机或可信局域网内学习，**不要部署到公网**：
> - 客户端→服务端**没有 TLS**，登录密码以明文 JSON 发送，链路上能被嗅探。
> - 服务端默认 `--bind 0.0.0.0`，没有任何鉴权之外的访问控制；公网暴露等于把数据库白送。
> - 测试账号 `alice / bob / carol`（密码 `123456`）会在数据库为空时自动注入，方便本地体验，但任何公网部署都需要先删掉这段逻辑。
> - SQLite 文件无加密；图片走 base64 in-band，单帧最大 ~1 MiB，没有做内容审计。
>
> 如要走向真实部署：套 TLS（QSslSocket / OpenSSL）、改成加盐挑战或一次性 token、关闭测试账号种子。

## 目标功能（最终形态）

以下功能会按阶段逐步实现（先做 MVP，再补齐）：
- 登录页面
- 注册页面
- 会话列表（会话切换）
- 用户聊天页面（单聊）
- 群聊页面
- 好友申请
- 群聊申请
- 选择头像
- 发送表情
- 图片发送
- 图片预览

## 分阶段路线图（建议）

### Phase 0：环境与工程骨架（必须先完成）
- 客户端/服务端都能编译、能启动
- 有最小运行/调试命令

### Phase 1：MVP（先把“聊天闭环”跑通）
以 `docs/requirements.md` 的验收标准为准：
- 登录（最简账号密码即可）
- 单聊文本收发（两端同时在线可互发）
- SQLite 落库 + 拉取最近 N 条历史

### Phase 2：接近微信的基础体验
- 注册页面
- 会话列表（至少 2 个会话切换）
- 好友申请（最简：申请/同意）
- 头像选择（本地资源即可）

### Phase 3：富媒体与群聊
- 发送表情
- 图片发送与预览（先本地/小图起步）
- 群聊页面（创建/加入/消息广播）
- 群聊申请（最简：申请/同意）

## 目录结构

```
TinyWeChat/
	protocol/       # 共享协议头（MsgType/ErrCode/FrameType），客户端 + 服务端都 include
	wx-client/      # Qt 客户端
	wx-server/      # Linux C++ 服务端
	docs/           # 需求/协议/存储/流程/环境
	AGENTS.md       # 项目专用"角色设定/规则入口"（给 Hermes 用）
	CLAUDE.md       # 通用行为准则（谨慎/简单/外科式修改/可验证）
```

## 服务端 (Phase 1 — 已实现)

服务端已完成 MVP：登录认证、单聊文本收发、消息落库、历史拉取。客户端已就绪，可直接启动 Qt GUI 交互。

### 构建

仓库顶层有统一 CMakeLists 同时构建客户端 + 服务端，推荐：

```bash
# 项目根目录
cmake -S . -B build -G Ninja
cmake --build build
# 产物：build/wx-server/tinywechat-server、build/wx-client/tinywechat-client
```

也可以单独构建任一端：

```bash
cmake -S wx-server -B wx-server/build -G Ninja
cmake --build wx-server/build
```

### 启动

```bash
./build/wx-server/tinywechat-server
```

**命令行参数：**

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--port 9090` | 9090 | 监听端口 |
| `--db data/tinywechat.db` | data/tinywechat.db | SQLite 数据库路径 |
| `--bind 0.0.0.0` | 0.0.0.0 | 绑定地址 |

数据库首次启动时自动创建，并预置三个测试用户：

| 用户名 | 密码 | 昵称 |
|--------|------|------|
| alice | 123456 | Alice |
| bob | 123456 | Bob |
| carol | 123456 | Carol |

### 用测试脚本验证

```bash
# 启动服务端（另一个终端）
./build/wx-server/tinywechat-server --port 19090

# 进入测试脚本所在目录
cd wx-server

# 运行功能验收（登录→发消息→收消息→拉历史）
python3 test_phase1.py --port 19090

# 运行 Phase 2/3 / 会话 / 边界测试
python3 test_phase2.py --port 19090
python3 test_phase3.py --port 19090
python3 test_conversations.py --port 19090
python3 test_edge.py --port 19090
```

### 协议概览

客户端与服务端通过 **TCP + 8 字节二进制帧头 + JSON Payload** 通信，详见 `docs/protocol.md`。

**帧格式（8 字节头）：**

```
Offset  Size  Field
────────────────────
0       4     length       Payload 长度（大端 uint32）
4       2     version      固定 0x0001
6       1     frame_type   0x01=请求  0x02=响应  0x03=通知  0x04=心跳
7       1     flags        预留，填 0x00
```

**Payload 结构（JSON）：**

```json
{"type": "消息类型", "seq": 序号, "body": { ... }}
```

**客户端交互流程：**

```
1. TCP 连接 → 服务端 IP:端口
2. 发送 auth.login 帧 → 收到 auth.login_res (code=0 表示成功)
3. 发送 chat.send 帧 → 收到 chat.send_res (发送确认)
4. 对方在线时收到 chat.recv 通知 (frame_type=0x03)
5. 发送 chat.history 帧 → 收到 chat.history_res (历史消息列表)
6. 每 30 秒发送 ping 心跳
```

**核心消息 type 速查（Phase 1）：**

| type | 方向 | 说明 |
|------|------|------|
| `auth.login` | C→S | 登录 `{username, password}` |
| `auth.login_res` | S→C | 登录结果 `{code, user_id, nickname}` |
| `chat.send` | C→S | 发消息 `{to_user_id, content, msg_type}` |
| `chat.send_res` | S→C | 发送确认 `{code, msg_id, timestamp}` |
| `chat.recv` | S→C | 消息推送（通知） |
| `chat.history` | C→S | 拉历史 `{with_user_id, before_msg_id, limit}` |
| `chat.history_res` | S→C | 历史列表 `{code, messages[]}` |
| `ping` / `pong` | C⇄S | 心跳 |

客户端实现时参考 `docs/protocol.md` 获取完整字段定义和错误码。

## 客户端 (Phase 1 — 已实现)

Qt6 Widgets 客户端，提供登录界面和聊天界面。架构分三层：网络层（帧编解码 + TCP 长连接）、协议层（消息构造/解析）、UI 层（登录页 + 聊天页）。

### 构建

推荐从项目根目录统一构建（同时产出客户端 + 服务端）：

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

或单独构建客户端：

```bash
cmake -S wx-client -B wx-client/build -G Ninja
cmake --build wx-client/build
```

**依赖：** Qt6 (Widgets, Network)、CMake ≥ 3.20、C++17。

Ubuntu 一键安装依赖：
```bash
sudo apt-get install -y qt6-base-dev cmake g++ ninja-build
```

macOS（Homebrew）：
```bash
brew install qt cmake ninja openssl nlohmann-json
```

### 启动

客户端默认连接 `127.0.0.1:9090`，因此必须先启动服务端：

```bash
# 终端 1：启动服务端
./build/wx-server/tinywechat-server --port 9090

# 终端 2：启动客户端（Alice）
./build/wx-client/tinywechat-client

# 终端 3：启动第二个客户端（Bob）
./build/wx-client/tinywechat-client
```

### 与服务端交互（操作指南）

客户端启动后，按以下步骤完成一次完整的聊天交互：

**1. 登录**

- 窗口显示「TinyWeChat 登录」标题
- 输入用户名和密码（测试用户：`alice` / `123456` 或 `bob` / `123456`）
- 点击「登录」按钮
- 成功：自动跳转到聊天页面；失败：显示错误提示

**2. 发起聊天**

- 聊天页面顶部有一个「发送给用户ID:」输入框
- 输入目标用户的 ID（Alice 的 ID 是 `1`，Bob 的 ID 是 `2`）
- 输入后可按回车确认

**3. 发送消息**

- 底部输入框输入消息内容（文本最长 4096 字节；图片走附件按钮，最大 ~700 KiB base64）
- 点击「发送」按钮或按回车键
- 消息立即显示在消息列表中（蓝色气泡 = 自己发的，绿色气泡 = 收到的）

**4. 接收消息**

- 对方发来的消息会实时推送到消息列表
- 推送消息以绿色气泡展示，带有发送者昵称

**5. 拉取历史消息**

在聊天页面输入目标用户 ID 后，客户端会自动向服务端请求与该用户的最近 20 条历史消息并展示。

### 交互流程图

```
┌──────────┐          ┌──────────┐          ┌──────────┐
│  Alice   │          │  服务端   │          │   Bob    │
│ (客户端)  │          │  :9090   │          │ (客户端)  │
└────┬─────┘          └────┬─────┘          └────┬─────┘
     │                     │                     │
     │── TCP connect ─────>│<─── TCP connect ────│
     │                     │                     │
     │── auth.login ──────>│                     │
     │<─ auth.login_res ───│                     │
     │   (code=0, uid=1)   │                     │
     │                     │<── auth.login ──────│
     │                     │── auth.login_res ──>│
     │                     │   (code=0, uid=2)   │
     │                     │                     │
     │── chat.send ───────>│                     │
     │   (to=2, "你好 Bob") │                     │
     │                     │── INSERT messages──>│
     │<─ chat.send_res ────│                     │
     │   (msg_id=42)       │                     │
     │                     │── chat.recv ───────>│
     │                     │   ("你好 Bob")       │
     │                     │                     │
     │                     │<── chat.send ───────│
     │                     │   (to=1, "Hi Alice")│
     │<─ chat.recv ────────│                     │
     │   ("Hi Alice")      │                     │
     │                     │                     │
```

### 文件结构

```
wx-client/
├── main.cpp                     # 入口
├── CMakeLists.txt               # 构建配置
├── network/
│   ├── FrameCodec.h / .cpp      # 8B 帧头 + JSON 编解码
│   └── WeChatSocket.h / .cpp    # TCP 长连接、帧收发、30s 心跳
├── protocol/
│   └── MessageBuilder.h / .cpp  # 消息构造 (login/send/history) 与响应解析
└── ui/
    ├── LoginWidget.h / .cpp      # 登录页面
    ├── RegisterWidget.h / .cpp   # 注册页面 (Phase 2)
    ├── ConversationListWidget.h / .cpp  # 会话列表 (Phase 2)
    ├── FriendListWidget.h / .cpp        # 好友管理 (Phase 2)
    ├── GroupListWidget.h / .cpp         # 群聊列表 (Phase 3)
    ├── GroupCreateDialog.h / .cpp       # 创建群聊对话框 (Phase 3)
    ├── EmojiPicker.h / .cpp             # 表情选择器 (Phase 3)
    ├── ChatWidget.h / .cpp       # 单聊页面
    ├── GroupChatWidget.h / .cpp  # 群聊页面 (Phase 3)
    ├── MainHubWidget.h / .cpp    # 标签页导航 (Phase 2)
    └── MainWindow.h / .cpp       # 主窗口（页面切换管理）
```

## Phase 2/3 新增功能（已实现）

### Phase 2 — 基础体验

**注册页面**
- 点击登录页的「还没有账号？立即注册」进入
- 填写用户名、昵称、密码后注册
- 注册成功返回登录页

**会话列表**
- 登录后默认显示「聊天」标签页
- 按最近时间排序展示所有单聊会话
- 点击任意会话进入聊天

**好友管理**
- 切换到「好友」标签页
- 显示好友列表，双击好友可发起聊天
- 「+ 添加好友」按钮：输入对方用户 ID 发送好友申请
- 待处理申请列表：双击可同意

**标签页导航**
- 顶部三标签：聊天 | 好友 | 群聊
- 登录后直接进入标签页主界面
- 从聊天页可点击「← 返回」回到主界面

### Phase 3 — 富媒体与群聊

**群聊**
- 切换到「群聊」标签页
- 「+ 创建群聊」创建新群
- 输入群 ID 加入已有群聊
- 双击群聊进入群消息页面
- 群消息实时广播给在线成员
- 支持群历史消息拉取

**表情发送**
- 聊天输入框左侧「😊」按钮
- 点击弹出 Unicode 表情面板（含 80+ 表情）
- 点击表情插入到输入框

**图片发送**
- 输入框左侧「📎」按钮
- 打开文件选择器（png/jpg/gif/bmp）
- 图片路径作为消息发送（msg_type=3）

## 文档入口（先看这些）

- 需求与验收：docs/requirements.md
- 环境搭建：docs/dev-setup.md
- 协议设计：docs/protocol.md
- 数据库设计：docs/storage.md
- 端到端时序：docs/flows.md
- Hermes 工作流：docs/hermes-workflow.md

## 作为初学者：推荐的实现顺序（最重要）

你只需要按下面 4 件事循环推进：
1) **读验收标准**（这一阶段“做完算什么”）
2) **让 Hermes 产出/更新规格文档**（protocol/storage/flows）
3) **实现最小代码**（外科式改动、先跑通再优化）
4) **用命令验证**（能跑、能联调、能复现）

## 快速开始（新手推荐）

1) 装依赖并自检
- 按 docs/dev-setup.md 安装 Qt/CMake/SQLite 等依赖

可选但推荐（隔离项目配置，避免与其它项目串）：
```bash
hermes profile create tinywechat --clone
hermes profile use tinywechat
```

2) 用 Hermes 先产出“协议/存储/时序”文档（主线只写文档，不写代码）
```bash
cd /home/jack/Projects/TinyWeChat
hermes chat --checkpoints

# 退出后，把刚创建的会话重命名为 tinywechat-arch（只需做一次）
hermes sessions list
hermes sessions rename <SESSION_ID> tinywechat-arch

# 以后继续这个会话再用：
hermes chat --checkpoints -c tinywechat-arch
```

3) 再并行实现服务端与客户端（推荐使用 worktree，等价“子 agent”隔离上下文）
```bash
# 第一次创建（各执行一次）：
hermes chat --checkpoints --worktree
hermes sessions list
hermes sessions rename <SESSION_ID> tinywechat-server

hermes chat --checkpoints --worktree
hermes sessions list
hermes sessions rename <SESSION_ID> tinywechat-client

# 以后继续再用：
hermes chat --checkpoints --worktree -c tinywechat-server
hermes chat --checkpoints --worktree -c tinywechat-client
```

## “AI 团队”协作方式（你可以照着分工开会话）

把下面 4 个会话当成 4 个不同专家：
- `tinywechat-arch`：架构/规格/验收负责人（只写 docs，少写代码）
- `tinywechat-server`：C++ 服务端负责人（协议、并发、落库、接口）
- `tinywechat-client`：Qt Widgets 客户端负责人（UI、网络、展示）
- `tinywechat-integration`：联调/验收负责人（跑命令、查日志、对齐行为）

建议固定命令（在仓库根目录运行）：

> 说明：下面这组命令用于“继续已命名会话”。如果你还没创建/命名这些会话，请先按上面的“第一次创建 → `hermes sessions rename`”流程做一次。

```bash
cd /home/jack/Projects/TinyWeChat
hermes chat --checkpoints -c tinywechat-arch
hermes chat --checkpoints --worktree -c tinywechat-server
hermes chat --checkpoints --worktree -c tinywechat-client
hermes chat --checkpoints -c tinywechat-integration
```

## Hermes 对话指南（如何“正确对话”一步一步做完）

### 1) 如何让 Hermes 持续遵循 AGENTS.md / CLAUDE.md

- **从仓库根目录启动会话**：确保 Hermes 能看到并注入本项目规则文件。
```bash
cd /home/jack/Projects/TinyWeChat
hermes chat --checkpoints -c tinywechat-arch
```
- **不要使用** `--ignore-rules`（否则会跳过自动注入的规则文件）。
- **每个会话第一句话**就明确要求：先阅读规则与需求，再开始动代码。

可复制的“通用开场提示词”（建议每个会话都粘贴一次）：

> 请先阅读 `AGENTS.md`、`CLAUDE.md`、`docs/requirements.md`，然后：
> 1) 用 3–5 条 bullet 复述你将遵守的关键约束；
> 2) 给出本会话的 3 步计划（每步都包含“验证方式/命令”）；
> 3) 只有在验收标准明确后才开始写代码。

### 2) docs/ 下的文件会自动识别吗？

- 一般来说：Hermes 会自动注入类似 `AGENTS.md` 这类“规则入口文件”；但 **`docs/` 里的具体文档不会被自动逐篇阅读**。
- 最稳妥做法：在会话里显式要求它阅读你关心的文档（例如 `docs/requirements.md`、后续生成的 `docs/protocol.md` 等）。

### 3) 推荐的三条会话（主线 + 两条并行线）

1) 架构/文档主线（只写 docs，不写代码）
```bash
hermes chat --checkpoints -c tinywechat-arch
```
建议你在该会话里直接说：

> 请基于 `docs/requirements.md`：
> - 产出协议草案到 `docs/protocol.md`（消息类型/字段/错误码/帧格式）
> - 产出数据库设计到 `docs/storage.md`
> - 产出端到端时序到 `docs/flows.md`
> - 给出 MVP 联调验收步骤（命令序列）

2) 服务端实现线（建议 worktree，避免与客户端互相改文件）
```bash
hermes chat --checkpoints --worktree -c tinywechat-server
```
建议你在该会话里说：

> 请先阅读 `docs/protocol.md` 和 `docs/storage.md`，再实现最小可运行服务端：
> - 能监听端口
> - 能处理 login
> - 能转发单聊文本消息
> - 每一步给出构建/运行/验证命令

3) 客户端实现线（同样建议 worktree）
```bash
hermes chat --checkpoints --worktree -c tinywechat-client
```
建议你在该会话里说：

> 请先阅读 `docs/protocol.md`，再实现最小 Qt 客户端：
> - 登录窗口
> - 聊天窗口（消息列表 + 输入框 + 发送按钮）
> - 能连接服务端并按协议收发
> - 每一步给出构建/运行/验证命令

## 分阶段“手把手”实施清单（新手照着做）

下面按 docs/requirements.md 的 Phase 0→3 拆解。你不需要一次做完所有功能：每完成一个 Phase，就把它跑通并验收。

### Phase 0：环境与工程骨架（先让一切能编译）

目标：服务端/客户端工程都能构建、能启动；并产出 3 份规格文档。

你对 `tinywechat-arch` 说（复制粘贴即可）：

> 请阅读 `AGENTS.md`、`CLAUDE.md`、`docs/requirements.md`。
> 仅完成 Phase 0：
> 1) 生成 `docs/protocol.md`（TCP 长度前缀帧 + JSON；消息类型覆盖 Phase 1 MVP）
> 2) 生成 `docs/storage.md`（SQLite 表结构覆盖 Phase 1 MVP）
> 3) 生成 `docs/flows.md`（登录/发消息/拉历史的时序）
> 4) 给出服务端/客户端工程骨架的目录与 CMake 方案（Qt Widgets + CMake + Ninja）

验收方式（你用命令验收）：
- `docs/protocol.md`/`docs/storage.md`/`docs/flows.md` 文件存在且自洽
- 服务端/客户端至少能编译出可执行文件（即使功能还没写完）

### Phase 1：MVP（聊天闭环：登录 + 单聊文本 + 历史）

目标：两端同时在线可互发文本；消息落库；登录后能拉取最近 20 条。

你对 `tinywechat-server` 说：

> 请先阅读 `docs/protocol.md`、`docs/storage.md`、`docs/flows.md`。
> 只实现 Phase 1：
> - 多客户端同时在线：能管理多个 TCP 连接
> - login：最简账号密码（可先内置 2 个测试用户）
> - send_message：单聊文本转发
> - SQLite：消息落库 + 拉取最近 20 条
> 每个小步骤给出构建/运行/验证命令。

你对 `tinywechat-client` 说：

> 请先阅读 `docs/protocol.md` 和 `docs/flows.md`。
> 只实现 Phase 1：
> - 登录窗口（Widgets）
> - 聊天窗口（消息列表 + 输入框 + 发送按钮）
> - 能连接服务端、登录、发文本、收文本
> - 登录后拉取最近 20 条并显示
> 每个小步骤给出构建/运行/验证命令。

你对 `tinywechat-integration` 说：

> 请给出一份从零到验收 Phase 1 的命令清单：
> - 如何启动服务端
> - 如何启动 2 个客户端实例（A/B）
> - A 发消息给 B 的验证步骤
> - B 离线再上线拉历史的验证步骤

### Phase 2：基础体验（注册 + 会话列表 + 好友申请 + 头像）

目标：接近“能用”的基础微信体验。

推进建议：每次只加 1 个功能点，并同步更新 docs/flows.md（行为）与 docs/storage.md（若涉及表结构）。
- 注册页面 + 注册接口
- 会话列表（至少 2 个会话切换）
- 好友申请（申请/同意）
- 头像选择与展示

### Phase 3：富媒体与群聊（表情 + 图片 + 群聊 + 群聊申请）

目标：实现更像微信的聊天内容与群聊。

推进建议：先“限制范围”再逐步放开（新手更稳）：
- 表情：优先做“表情 ID → 内置资源”的方案
- 图片：先限制大小与类型；先做发送与预览
- 群聊：先做创建/加入/广播；后续再做更复杂的成员管理
