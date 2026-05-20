#pragma once

#include <cstdint>
#include <string>
#include <QByteArray>
#include <QJsonObject>

// ── Frame constants ─────────────────────────────────────
constexpr uint16_t PROTOCOL_VERSION = 0x0001;
constexpr size_t   FRAME_HEADER_SIZE = 8;
constexpr size_t   MAX_PAYLOAD_SIZE  = 65535;

enum class FrameType : uint8_t {
    REQUEST      = 0x01,
    RESPONSE     = 0x02,
    NOTIFICATION = 0x03,
    PINGPONG     = 0x04,
};

// ── Message types ──────────────────────────────────────
namespace MsgType {
    inline const QString LOGIN     = QStringLiteral("auth.login");
    inline const QString LOGIN_RES = QStringLiteral("auth.login_res");
    inline const QString REGISTER     = QStringLiteral("auth.register");
    inline const QString REGISTER_RES = QStringLiteral("auth.register_res");
    inline const QString SEND        = QStringLiteral("chat.send");
    inline const QString SEND_RES    = QStringLiteral("chat.send_res");
    inline const QString RECV        = QStringLiteral("chat.recv");
    inline const QString HISTORY     = QStringLiteral("chat.history");
    inline const QString HISTORY_RES = QStringLiteral("chat.history_res");
    inline const QString FRIEND_REQUEST     = QStringLiteral("friend.request");
    inline const QString FRIEND_REQUEST_RES = QStringLiteral("friend.request_res");
    inline const QString FRIEND_NOTIFY      = QStringLiteral("friend.notify");
    inline const QString FRIEND_HANDLE      = QStringLiteral("friend.handle");
    inline const QString FRIEND_HANDLE_RES  = QStringLiteral("friend.handle_res");
    inline const QString FRIEND_LIST        = QStringLiteral("friend.list");
    inline const QString FRIEND_LIST_RES    = QStringLiteral("friend.list_res");
    inline const QString FRIEND_PENDING     = QStringLiteral("friend.pending");
    inline const QString FRIEND_PENDING_RES = QStringLiteral("friend.pending_res");
    inline const QString GROUP_CREATE     = QStringLiteral("group.create");
    inline const QString GROUP_CREATE_RES = QStringLiteral("group.create_res");
    inline const QString GROUP_JOIN       = QStringLiteral("group.join");
    inline const QString GROUP_JOIN_RES   = QStringLiteral("group.join_res");
    inline const QString GROUP_SEND       = QStringLiteral("group.send");
    inline const QString GROUP_SEND_RES   = QStringLiteral("group.send_res");
    inline const QString GROUP_RECV       = QStringLiteral("group.recv");
    inline const QString GROUP_HISTORY    = QStringLiteral("group.history");
    inline const QString GROUP_HISTORY_RES = QStringLiteral("group.history_res");
    inline const QString GROUP_LIST       = QStringLiteral("group.list");
    inline const QString GROUP_LIST_RES   = QStringLiteral("group.list_res");
    inline const QString PING = QStringLiteral("ping");
    inline const QString PONG = QStringLiteral("pong");
}

// ── Error codes ────────────────────────────────────────
namespace ErrCode {
    constexpr int SUCCESS = 0;
    constexpr int INVALID_REQUEST = 200;
    constexpr int UNAUTHORIZED = 210;
}

// ── Protocol functions ─────────────────────────────────

/// Serialize a frame: 8-byte header (big-endian) + JSON payload.
QByteArray buildFrame(const QString& type, const QJsonObject& body,
                      int seq, FrameType frameType = FrameType::REQUEST);

/// Try to parse one frame from buffer. Returns empty if incomplete.
/// Modifies buffer in-place (removes consumed data).
QByteArray parseFrame(QByteArray& buffer, FrameType& outType,
                      QJsonObject& outPayload);
