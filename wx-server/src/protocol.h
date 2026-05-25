#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "protocol/common.h"   // 共享：FrameType / MsgType / ErrCode / 帧大小常量

using json = nlohmann::json;

// ── Parsed frame（服务端版本：payload 为 JSON 字符串）────
struct Frame {
    uint32_t length     = 0;
    uint16_t version    = PROTOCOL_VERSION;
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
/// When seq > 0, uses RESPONSE frame type so client can match the error to its request.
Frame buildError(int code, const std::string& msg, int seq = 0);
