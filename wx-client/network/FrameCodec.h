#pragma once

#include <cstdint>
#include <QByteArray>
#include <QJsonObject>

#include "protocol/common.h"   // 共享：FrameType / 帧大小常量

// 解码后的帧结构（客户端版本：payload 为 QJsonObject）
struct Frame {
    uint16_t   version   = PROTOCOL_VERSION;
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
    enum class DecodeResult {
        Ok,           // 成功解码一帧，consumed 有效
        Incomplete,   // 数据不足，等更多数据
        Malformed,    // 帧格式损坏，应断开连接
    };

    // 将 Frame 编码为网络字节流；失败返回空 QByteArray
    static QByteArray encode(const Frame& frame);

    // 从字节流中解码一帧。返回值见 DecodeResult。
    static DecodeResult decode(const QByteArray& data, Frame& frame, int& consumed);
};
