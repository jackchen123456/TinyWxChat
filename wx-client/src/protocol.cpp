#include "protocol.h"
#include <QJsonDocument>
#include <QDataStream>
#include <QIODevice>

QByteArray buildFrame(const QString& type, const QJsonObject& body,
                      int seq, FrameType frameType)
{
    QJsonObject root;
    root["type"] = type;
    root["seq"]  = seq;
    root["body"] = body;

    QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Compact);
    quint32 len = static_cast<quint32>(payload.size());

    QByteArray frame(FRAME_HEADER_SIZE, '\0');
    QDataStream stream(&frame, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << len;
    stream << quint16(PROTOCOL_VERSION);
    stream << quint8(static_cast<uint8_t>(frameType));
    stream << quint8(0);  // flags

    frame.append(payload);
    return frame;
}

QByteArray parseFrame(QByteArray& buffer, FrameType& outType,
                      QJsonObject& outPayload)
{
    if (buffer.size() < static_cast<int>(FRAME_HEADER_SIZE))
        return {};

    QDataStream stream(buffer);
    stream.setByteOrder(QDataStream::BigEndian);

    quint32 len; quint16 ver; quint8 type, flags;
    stream >> len >> ver >> type >> flags;
    outType = static_cast<FrameType>(type);

    int total = FRAME_HEADER_SIZE + len;
    if (buffer.size() < total)
        return {};  // incomplete

    QByteArray frame = buffer.left(total);
    buffer.remove(0, total);

    if (len > 0) {
        QByteArray payload = frame.mid(FRAME_HEADER_SIZE, len);
        QJsonDocument doc = QJsonDocument::fromJson(payload);
        if (doc.isObject())
            outPayload = doc.object();
    }
    return frame;
}
