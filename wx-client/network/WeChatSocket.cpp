#include "WeChatSocket.h"
#include <QDebug>

WeChatSocket::WeChatSocket(QObject* parent)
    : QTcpSocket(parent)
    , m_heartbeatTimer(new QTimer(this))
    , m_timeoutTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
{
    connect(this, &QIODevice::readyRead, this, &WeChatSocket::onReadyRead);

    m_heartbeatTimer->setInterval(30000);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &WeChatSocket::onHeartbeat);

    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &WeChatSocket::onConnectionTimeout);

    // 重连定时器（单次触发）：超时后真正发起 connectToHost
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!m_reconnecting) return;  // 取消
        qDebug() << "[WeChatSocket] connectToHost (reconnect)";
        connectToHost(m_host, m_port);
    });

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(this, &QTcpSocket::errorOccurred, this, &WeChatSocket::onSocketError);
#else
    connect(this, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &WeChatSocket::onSocketError);
#endif

    // 连接成功时：停止重连、启动心跳
    connect(this, &QTcpSocket::connected, this, [this]() {
        if (m_reconnecting) {
            qDebug() << "[WeChatSocket] Reconnect succeeded!";
            m_reconnecting = false;
            m_reconnectAttempt = 0;
            m_reconnectTimer->stop();
        }
        m_heartbeatTimer->start(30000);
        m_timeoutTimer->start(90000);
        m_buffer.clear();
    });
}

void WeChatSocket::connectToServer(const QString& host, quint16 port)
{
    m_host = host;
    m_port = port;
    m_reconnectAttempt = 0;
    m_reconnecting = false;
    m_reconnectTimer->stop();

    qDebug() << "[WeChatSocket] Connecting to" << host << ":" << port;
    connectToHost(host, port);
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

    // 写缓冲保护：若 QTcpSocket 内部待发字节 + 本帧 > MAX_WRITE_BUFFER，拒绝发送
    // 避免慢网络/对端卡死时本端内存被无限拉高
    if (bytesToWrite() + data.size() > MAX_WRITE_BUFFER) {
        qWarning() << "[WeChatSocket] Write buffer full ("
                   << bytesToWrite() << "+" << data.size()
                   << ">" << MAX_WRITE_BUFFER << "), dropping frame";
        emit connectionFailed("发送缓冲溢出，对端可能已卡住");
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

void WeChatSocket::onReadyRead()
{
    m_buffer.append(readAll());
    qDebug() << "[WeChatSocket] Buffer size:" << m_buffer.size();

    // 重置超时
    m_timeoutTimer->start(90000);

    while (true) {
        Frame frame;
        int consumed = 0;
        auto result = FrameCodec::decode(m_buffer, frame, consumed);
        if (result == FrameCodec::DecodeResult::Incomplete)
            break;
        if (result == FrameCodec::DecodeResult::Malformed) {
            qWarning() << "[WeChatSocket] Malformed frame, disconnecting";
            m_buffer.clear();
            abort();  // QTcpSocket::abort — 立即断开
            return;
        }

        m_buffer.remove(0, consumed);
        qDebug() << "[WeChatSocket] Decoded frame, type:" << static_cast<int>(frame.frameType);

        if (frame.frameType == FrameType::PINGPONG) {
            qDebug() << "[WeChatSocket] Received PONG";
            continue;
        }

        emit frameReceived(frame);
    }
}

void WeChatSocket::onHeartbeat()
{
    qDebug() << "[WeChatSocket] Sending PING";
    sendFrame(FrameType::PINGPONG, QJsonObject());
}

void WeChatSocket::onConnectionTimeout()
{
    qWarning() << "[WeChatSocket] Connection timeout (90s)";
    m_heartbeatTimer->stop();
    close();
    startReconnect();
}

void WeChatSocket::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    qWarning() << "[WeChatSocket] Socket error:" << errorString();
    m_heartbeatTimer->stop();
    m_timeoutTimer->stop();
    if (m_reconnecting) {
        // 若已有重连定时器在计时，则忽略本次错误（避免计数器重复递增）
        if (m_reconnectTimer->isActive()) return;
        attemptReconnect();
    } else {
        startReconnect();
    }
}

// ── 指数退避重连（1s / 2s / 4s / 8s / 16s，最多 5 次）──────────

void WeChatSocket::startReconnect()
{
    if (m_reconnecting) return;  // 已经在重连中
    if (m_host.isEmpty() || m_port == 0) {
        // 没有连接信息，无法重连
        emit connectionFailed("连接已断开（无重连目标）");
        return;
    }

    m_reconnecting = true;
    m_reconnectAttempt = 0;
    emit connectionFailed("连接断开，正在自动重连...");
    attemptReconnect();
}

void WeChatSocket::attemptReconnect()
{
    if (m_reconnectAttempt >= MAX_RECONNECT_ATTEMPTS) {
        qWarning() << "[WeChatSocket] Max reconnect attempts reached, giving up";
        m_reconnecting = false;
        m_reconnectTimer->stop();
        emit connectionFailed(QString("自动重连失败（已尝试 %1 次），请手动重新连接")
                                  .arg(MAX_RECONNECT_ATTEMPTS));
        return;
    }

    m_reconnectAttempt++;
    int delayMs = 1000 * (1 << (m_reconnectAttempt - 1));  // 1s, 2s, 4s, 8s, 16s
    qDebug() << "[WeChatSocket] Reconnect attempt" << m_reconnectAttempt
             << "/" << MAX_RECONNECT_ATTEMPTS << "in" << delayMs << "ms";

    emit reconnecting(m_reconnectAttempt, MAX_RECONNECT_ATTEMPTS);

    // 关闭旧连接（abort 不会触发 errorOccurred）
    abort();
    m_buffer.clear();

    // 延迟 delayMs 后再 connectToHost；
    // 若连接失败 onSocketError 会触发下一次 attemptReconnect。
    m_reconnectTimer->start(delayMs);
}

void WeChatSocket::stopReconnect()
{
    m_reconnecting = false;
    m_reconnectAttempt = 0;
    m_reconnectTimer->stop();
}
