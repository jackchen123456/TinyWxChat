#pragma once

#include <cstdint>
#include <QByteArray>
#include <QJsonObject>

// 帧类型（对应 protocol.md §1.1）
enum class FrameType : uint8_t {
    REQUEST      = 0x01,
    RESPONSE     = 0x02,
    NOTIFICATION = 0x03,
    PING_PONG    = 0x04
};

// 解码后的帧结构
struct Frame {
    uint16_t   version   = 0x0001;
    FrameType  frameType = FrameType::REQUEST;
    uint8_t    flags     = 0x00;
    QJsonObject payload;
};

// 帧编解码器：8B 二进制头 + JSON Payload
// 帧头格式（大端）：
//   Offset 0..3: uint32 length (Payload 长度，不含帧头)
//   Offset 4..5: uint16 version (固定 0x0001)
//   Offset 6:    uint8  frame_type
//   Offset 7:    uint8  flags
class FrameCodec {
public:
    // 将 Frame 编码为网络字节流；失败返回空 QByteArray
    static QByteArray encode(const Frame& frame);

    // 从字节流中解码一帧。
    // 返回 true 表示成功解码了一帧，consumed 为消费的字节数。
    // 返回 false 表示数据不足（等更多数据）或格式错误（应断开）。
    static bool decode(const QByteArray& data, Frame& frame, int& consumed);

    static constexpr int HEADER_SIZE       = 8;
    static constexpr uint32_t MAX_PAYLOAD  = 65535;  // 64 KiB
};
