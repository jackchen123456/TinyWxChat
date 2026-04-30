#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ── Frame header constants ───────────────────────────────
constexpr uint16_t PROTOCOL_VERSION = 0x0001;
constexpr size_t   FRAME_HEADER_SIZE = 8;    // 4+2+1+1
constexpr size_t   MAX_PAYLOAD_SIZE  = 65535;

enum class FrameType : uint8_t {
    REQUEST      = 0x01,
    RESPONSE     = 0x02,
    NOTIFICATION = 0x03,
    PINGPONG     = 0x04,
};

// ── Message type strings ─────────────────────────────────
namespace MsgType {
    // Auth
    constexpr const char* LOGIN     = "auth.login";
    constexpr const char* LOGIN_RES = "auth.login_res";

    // Chat
    constexpr const char* SEND        = "chat.send";
    constexpr const char* SEND_RES    = "chat.send_res";
    constexpr const char* RECV        = "chat.recv";
    constexpr const char* HISTORY     = "chat.history";
    constexpr const char* HISTORY_RES = "chat.history_res";

    // Heartbeat
    constexpr const char* PING = "ping";
    constexpr const char* PONG = "pong";

    // Error
    constexpr const char* ERROR = "error";
}

// ── Error codes ──────────────────────────────────────────
namespace ErrCode {
    constexpr int SUCCESS          = 0;
    constexpr int INTERNAL_ERROR   = 101;
    constexpr int INVALID_REQUEST  = 200;
    constexpr int INVALID_FRAME    = 201;
    constexpr int UNAUTHORIZED     = 210;
    constexpr int USER_NOT_FOUND   = 300;
    constexpr int USER_OFFLINE     = 301;
    constexpr int WRONG_PASSWORD   = 401;
    constexpr int MESSAGE_TOO_LONG = 500;
}

// ── Parsed frame ─────────────────────────────────────────
struct Frame {
    uint32_t length     = 0;
    uint16_t version    = 0;
    FrameType frameType = FrameType::REQUEST;
    uint8_t  flags      = 0;
    std::string payload;   // JSON string
};

// ── Protocol functions ───────────────────────────────────

/// Build an 8-byte header into buf (must be FRAME_HEADER_SIZE bytes).
void buildHeader(uint8_t* buf, uint32_t length, FrameType type,
                 uint8_t flags = 0, uint16_t version = PROTOCOL_VERSION);

/// Parse an 8-byte header. Returns false on invalid magic / length overflow.
bool parseHeader(const uint8_t* buf, Frame& out);

/// Serialize header + payload into a byte vector ready for send().
std::vector<uint8_t> serializeFrame(const Frame& frame);

/// Build a response frame with the given type, seq, and body.
Frame buildResponse(const std::string& type, int seq, const json& body);

/// Build a notification frame (always frameType=NOTIFICATION).
Frame buildNotification(const std::string& type, const json& body);

/// Build an error notification with code and human-readable message.
Frame buildError(int code, const std::string& msg);
