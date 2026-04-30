#include "WeChatSocket.h"
#include <QDebug>

WeChatSocket::WeChatSocket(QObject* parent)
    : QTcpSocket(parent)
    , m_heartbeatTimer(new QTimer(this))
    , m_timeoutTimer(new QTimer(this))
{
    // 收到数据时触发解码
    connect(this, &QIODevice::readyRead, this, &WeChatSocket::onReadyRead);

    // 心跳定时器：每 30s 发送 PING
    m_heartbeatTimer->setInterval(30000);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &WeChatSocket::onHeartbeat);

    // 超时定时器：90s 无任何数据则断开
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &WeChatSocket::onConnectionTimeout);

    // 套接字错误处理
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(this, &QTcpSocket::errorOccurred, this, &WeChatSocket::onSocketError);
#else
    connect(this, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &WeChatSocket::onSocketError);
#endif
}

void WeChatSocket::connectToServer(const QString& host, quint16 port)
{
    qDebug() << "[WeChatSocket] Connecting to" << host << ":" << port;
    connectToHost(host, port);
}

void WeChatSocket::onReadyRead()
{
    m_buffer.append(readAll());
    qDebug() << "[WeChatSocket] Buffer size:" << m_buffer.size();

    // 重置超时：收到任何数据都算活动
    m_timeoutTimer->start(90000);

    // 循环解码，直到缓冲区没有完整帧
    while (true) {
        Frame frame;
        int consumed = 0;
        if (!FrameCodec::decode(m_buffer, frame, consumed))
            break; // 数据不足 或 格式错误

        m_buffer.remove(0, consumed);
        qDebug() << "[WeChatSocket] Decoded frame, type:" << static_cast<int>(frame.frameType);

        // PING/PONG 内部处理（不暴露给上层）
        if (frame.frameType == FrameType::PING_PONG) {
            qDebug() << "[WeChatSocket] Received PONG";
            continue;
        }

        emit frameReceived(frame);
    }
}

bool WeChatSocket::sendFrame(FrameType type, const QJsonObject& body)
{
    if (!isConnected()) {
        qWarning() << "[WeChatSocket] Cannot send: not connected";
        return false;
    }

    Frame frame;
    frame.frameType = type;
    frame.payload = body;

    QByteArray data = FrameCodec::encode(frame);
    if (data.isEmpty()) {
        qWarning() << "[WeChatSocket] Frame encode failed";
        return false;
    }

    qint64 written = write(data);
    if (written != data.size()) {
        qWarning() << "[WeChatSocket] Write incomplete:" << written << "/" << data.size();
        return false;
    }

    qDebug() << "[WeChatSocket] Sent frame, type:" << static_cast<int>(type)
             << "size:" << data.size();
    return true;
}

void WeChatSocket::onHeartbeat()
{
    qDebug() << "[WeChatSocket] Sending PING";
    sendFrame(FrameType::PING_PONG, QJsonObject());
}

void WeChatSocket::onConnectionTimeout()
{
    qWarning() << "[WeChatSocket] Connection timeout (90s)";
    m_heartbeatTimer->stop();
    close();
    emit connectionFailed("连接超时（90 秒无响应）");
}

void WeChatSocket::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    qWarning() << "[WeChatSocket] Socket error:" << errorString();
    m_heartbeatTimer->stop();
    m_timeoutTimer->stop();
    emit connectionFailed(errorString());
}
