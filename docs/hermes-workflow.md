# 如何用 Hermes 分步实现 TinyWeChat

> 目标：把大项目拆成“可并行、可验收、低返工”的若干会话。

## 0. 建议：为项目创建独立 Hermes Profile（可选但推荐）

Profile 可以隔离模型、配置和会话习惯，避免和别的项目串。

- `hermes profile create tinywechat --clone`：新建一个名为 `tinywechat` 的独立 Profile，并把你当前正在用的 Profile 配置（如 config.yaml/.env 等）复制一份作为起点。
- `hermes profile use tinywechat`：把 `tinywechat` 设为“默认 Profile”（sticky）。之后你运行 `hermes chat`、`hermes sessions` 等命令会默认使用它，直到你切换到别的 Profile。
- 如果你不需要隔离（例如只做这一个项目），可以跳过 Profile 步骤，直接从第 1 节开始。

```bash
# 创建（从当前 active profile 克隆配置）
hermes profile create tinywechat --clone

# 设为默认（sticky）
hermes profile use tinywechat

# 查看/确认
hermes profile list
hermes profile show tinywechat
```

## 0.5 重要：确保规则与文档被遵循

本项目的“会话开场必做（强制）”规范以 `AGENTS.md` 为准。

操作上只记三点：
- 从仓库根目录启动会话：`cd /home/jack/Projects/TinyWeChat`
- 不要使用 `--ignore-rules`
- 每个会话开场先让 Agent 按 `AGENTS.md` 的清单确认已阅读相关文档

## 1. 主线会话：需求/协议/里程碑（只做文档，不写代码）

在仓库根目录运行：
```bash
cd /home/jack/Projects/TinyWeChat
hermes chat --checkpoints -c tinywechat-arch
```

建议你在会话里粘贴这个任务：
- “请基于 `docs/requirements.md`：
  1) 输出协议草案（消息类型、字段、错误码），写到 `docs/protocol.md`
  2) 输出数据库表结构，写到 `docs/storage.md`
  3) 输出端到端时序（登录/发消息/拉历史），写到 `docs/flows.md`
  4) 给出 MVP 的联调步骤（手工验收脚本）”

## 2. 并行会话：服务端与客户端各一条线

### 服务端（建议独立 worktree，避免并行互相改文件）
```bash
hermes chat --checkpoints --worktree -c tinywechat-server
```

服务端会话任务建议：
- “先阅读 `docs/protocol.md`、`docs/storage.md`、`docs/flows.md`，再按 Phase 1（MVP）实现服务端：
  - 能监听端口、可同时处理多个客户端连接
  - 支持 login（可先内置测试用户用于联调）
  - 支持单聊文本转发
  - SQLite 落库 + 拉取最近 N 条历史（默认 N=20）
  - 每一步都给出构建/运行/验证命令”

### 客户端（同样独立 worktree）
```bash
hermes chat --checkpoints --worktree -c tinywechat-client
```

客户端会话任务建议：
- “先阅读 `docs/protocol.md` 与 `docs/flows.md`，再按 Phase 1（MVP）实现最小 Qt Widgets 客户端：
  - 登录窗口
  - 聊天窗口（消息列表 + 输入框 + 发送按钮）
  - 能连接、登录、收发单聊文本
  - 登录后拉取最近 N 条历史并展示
  - 每一步都给出构建/运行/验证命令”

## 3. 集成会话：联调与验收

建议单独开一个“集成/验收”会话，专门负责联调脚本与问题定位：
```bash
hermes chat --checkpoints -c tinywechat-integration
```

让它执行：
- 对照 `docs/requirements.md` 的验收标准逐条检查（先 Phase 0，再 Phase 1）
- 输出一份“从零到跑通”的命令序列（build server/client → run → 双端收发验证）
- 当联调失败时：先给出最短排查路径（看哪几行日志/抓哪一段报文/最可能的 3 个原因）

## 4. 常用命令

- 查看会话：`hermes sessions list`
- 继续会话：`hermes chat -c tinywechat-server`（或 client/arch/integration）
- 如果你想脚本化：用 `hermes -z "..."` 做一次性生成（只输出最终文本）
