# TinyWeChat

一个“仿微信”的练手项目：**Qt6 客户端** + **Linux C++ 服务端**。

定位：面向 C++/Qt 新手，强调“先跑通、可验收、少返工”。仓库内文档作为单一真相源，后续代码实现全部围绕验收标准推进。

如果你是初学者：把 Hermes 的“多会话 + worktree”当成一个 AI 团队（架构/后端/客户端/联调各一位专家），按本文一步一步推进即可。

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
	wx-client/      # Qt 客户端
	wx-server/      # Linux C++ 服务端
	docs/           # 需求/协议/存储/流程/环境
	AGENTS.md       # 项目专用“角色设定/规则入口”（给 Hermes 用）
	CLAUDE.md       # 通用行为准则（谨慎/简单/外科式修改/可验证）
```

## 文档入口（先看这些）

- 需求与验收：docs/requirements.md
- 环境搭建：docs/dev-setup.md
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
hermes chat --checkpoints -c tinywechat-arch
```

3) 再并行实现服务端与客户端（推荐使用 worktree，等价“子 agent”隔离上下文）
```bash
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
