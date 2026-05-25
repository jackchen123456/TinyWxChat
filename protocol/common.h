#pragma once

// TinyWeChat 共享协议头：客户端 / 服务端共同 include。
// 只放纯协议常量、枚举、错误码 —— 不依赖 Qt、不依赖 nlohmann_json。
//
// 设计目标：把"客户端/服务端常量必须一致"从口头约定升级为编译期保证。
// 任一端误写常量名/数值，构建直接失败。
//
// 不在此处的协议元素：
//  - 服务端 Frame 结构（payload 是 std::string） → wx-server/src/protocol.h
//  - 客户端 Frame 结构（payload 是 QJsonObject）+ FrameCodec → wx-client/network/FrameCodec.h
//  - Python 测试脚本仍硬编码字面量；它们不参与 C++ 编译，无法享受这层保护。

#include <cstdint>
#include <cstddef>

// ── 帧头常量 ─────────────────────────────────────────────
constexpr uint16_t PROTOCOL_VERSION  = 0x0001;
constexpr size_t   FRAME_HEADER_SIZE = 8;                  // 4+2+1+1
constexpr size_t   MAX_PAYLOAD_SIZE  = 1024 * 1024;        // 1 MiB（足够 base64 ~700KB 图片）

// ── 帧类型 ───────────────────────────────────────────────
enum class FrameType : uint8_t {
    REQUEST      = 0x01,
    RESPONSE     = 0x02,
    NOTIFICATION = 0x03,
    PINGPONG     = 0x04,
};

// ── 消息类型字符串 ───────────────────────────────────────
namespace MsgType {
    // ── Auth ─────────────────────────────────────────────
    constexpr const char* LOGIN         = "auth.login";
    constexpr const char* LOGIN_RES     = "auth.login_res";
    constexpr const char* REGISTER      = "auth.register";
    constexpr const char* REGISTER_RES  = "auth.register_res";

    // ── Chat ─────────────────────────────────────────────
    constexpr const char* SEND              = "chat.send";
    constexpr const char* SEND_RES          = "chat.send_res";
    constexpr const char* RECV              = "chat.recv";
    constexpr const char* HISTORY           = "chat.history";
    constexpr const char* HISTORY_RES       = "chat.history_res";
    constexpr const char* CONVERSATIONS     = "chat.conversations";
    constexpr const char* CONVERSATIONS_RES = "chat.conversations_res";

    // ── Heartbeat ────────────────────────────────────────
    constexpr const char* PING = "ping";
    constexpr const char* PONG = "pong";

    // ── Friend ───────────────────────────────────────────
    constexpr const char* FRIEND_REQUEST     = "friend.request";
    constexpr const char* FRIEND_REQUEST_RES = "friend.request_res";
    constexpr const char* FRIEND_NOTIFY      = "friend.notify";
    constexpr const char* FRIEND_HANDLE      = "friend.handle";
    constexpr const char* FRIEND_HANDLE_RES  = "friend.handle_res";
    constexpr const char* FRIEND_LIST        = "friend.list";
    constexpr const char* FRIEND_LIST_RES    = "friend.list_res";
    constexpr const char* FRIEND_PENDING     = "friend.pending";
    constexpr const char* FRIEND_PENDING_RES = "friend.pending_res";

    // ── Group ────────────────────────────────────────────
    constexpr const char* GROUP_CREATE           = "group.create";
    constexpr const char* GROUP_CREATE_RES       = "group.create_res";
    constexpr const char* GROUP_JOIN             = "group.join";
    constexpr const char* GROUP_JOIN_RES         = "group.join_res";
    constexpr const char* GROUP_SEND             = "group.send";
    constexpr const char* GROUP_SEND_RES         = "group.send_res";
    constexpr const char* GROUP_RECV             = "group.recv";
    constexpr const char* GROUP_HISTORY          = "group.history";
    constexpr const char* GROUP_HISTORY_RES      = "group.history_res";
    constexpr const char* GROUP_LIST             = "group.list";
    constexpr const char* GROUP_LIST_RES         = "group.list_res";
    constexpr const char* GROUP_APPLY            = "group.apply";
    constexpr const char* GROUP_APPLY_RES        = "group.apply_res";
    constexpr const char* GROUP_APPLY_NOTIFY     = "group.apply_notify";
    constexpr const char* GROUP_APPLY_HANDLE     = "group.apply_handle";
    constexpr const char* GROUP_APPLY_HANDLE_RES = "group.apply_handle_res";

    // ── Error ────────────────────────────────────────────
    constexpr const char* ERROR = "error";
}

// ── 错误码 ───────────────────────────────────────────────
namespace ErrCode {
    constexpr int SUCCESS                 = 0;
    constexpr int INTERNAL_ERROR          = 101;
    constexpr int INVALID_REQUEST         = 200;
    constexpr int INVALID_FRAME           = 201;
    constexpr int UNAUTHORIZED            = 210;
    constexpr int USER_NOT_FOUND          = 300;
    constexpr int USER_OFFLINE            = 301;
    constexpr int FRIEND_NOT_FOUND        = 310;
    constexpr int FRIEND_ALREADY          = 311;
    constexpr int FRIEND_REQUEST_EXISTS   = 312;
    constexpr int GROUP_NOT_FOUND         = 320;
    constexpr int GROUP_ALREADY_MEMBER    = 321;
    constexpr int GROUP_PERMISSION_DENIED = 322;
    constexpr int DUPLICATE_USERNAME      = 400;
    constexpr int WRONG_PASSWORD          = 401;
    constexpr int USERNAME_TOO_LONG       = 402;
    constexpr int MESSAGE_TOO_LONG        = 500;
    constexpr int RATE_LIMITED            = 501;
}
