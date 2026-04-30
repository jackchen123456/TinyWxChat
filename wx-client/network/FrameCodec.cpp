#include "FrameCodec.h"
#include <QJsonDocument>
#include <cstring>

QByteArray FrameCodec::encode(const Frame& frame)
{
    // 1. 序列化 payload 为 JSON
    QByteArray payload = QJsonDocument(frame.payload).toJson(QJsonDocument::Compact);
    if (static_cast<uint32_t>(payload.size()) > MAX_PAYLOAD) {
        return {}; // Payload 超限
    }

    // 2. 构造 8B 帧头 + payload
    QByteArray result;
    result.resize(HEADER_SIZE + payload.size());

    uint32_t length = static_cast<uint32_t>(payload.size());

    // length (大端 uint32)
    result[0] = static_cast<char>((length >> 24) & 0xFF);
    result[1] = static_cast<char>((length >> 16) & 0xFF);
    result[2] = static_cast<char>((length >> 8)  & 0xFF);
    result[3] = static_cast<char>( length        & 0xFF);

    // version (大端 uint16)
    result[4] = static_cast<char>((frame.version >> 8) & 0xFF);
    result[5] = static_cast<char>( frame.version       & 0xFF);

    // frame_type
    result[6] = static_cast<char>(frame.frameType);

    // flags
    result[7] = static_cast<char>(frame.flags);

    // payload
    if (length > 0) {
        std::memcpy(result.data() + HEADER_SIZE, payload.constData(), length);
    }

    return result;
}

bool FrameCodec::decode(const QByteArray& data, Frame& frame, int& consumed)
{
    // 1. 至少需要 8B 帧头
    if (data.size() < HEADER_SIZE)
        return false; // 数据不足

    // 2. 解析 length（大端 uint32）
    uint32_t length = (static_cast<uint8_t>(data[0]) << 24)
                    | (static_cast<uint8_t>(data[1]) << 16)
                    | (static_cast<uint8_t>(data[2]) << 8)
                    |  static_cast<uint8_t>(data[3]);

    if (length > MAX_PAYLOAD)
        return false; // 帧格式错误：length 超限

    // 3. 检查 payload 是否已完整到达
    if (data.size() < static_cast<int>(HEADER_SIZE + length))
        return false; // 数据不足（等更多数据）

    // 4. 解析 version、frame_type、flags
    frame.version   = (static_cast<uint8_t>(data[4]) << 8)
                    |  static_cast<uint8_t>(data[5]);
    frame.frameType = static_cast<FrameType>(data[6]);
    frame.flags     = static_cast<uint8_t>(data[7]);

    // 5. 解析 JSON payload
    if (length > 0) {
        QByteArray payloadBytes = data.mid(HEADER_SIZE, static_cast<int>(length));
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(payloadBytes, &err);
        if (err.error != QJsonParseError::NoError)
            return false; // JSON 解析失败
        frame.payload = doc.object();
    } else {
        frame.payload = QJsonObject(); // 空 payload（如 PING/PONG）
    }

    consumed = HEADER_SIZE + static_cast<int>(length);
    return true;
}
