#pragma once

#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include "FrameCodec.h"

class WeChatSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit WeChatSocket(QObject* parent = nullptr);

    void connectToServer(const QString& host, quint16 port);
    bool sendFrame(FrameType type, const QJsonObject& body);
    uint32_t nextSeq() { return m_seq++; }
    bool isConnected() const { return state() == QAbstractSocket::ConnectedState; }

    // 重连状态
    bool isReconnecting() const { return m_reconnecting; }

signals:
    void frameReceived(const Frame& frame);
    void connectionFailed(const QString& reason);
    // 重连中（UI 可显示"正在重连..."）
    void reconnecting(int attempt, int maxAttempts);

private slots:
    void onReadyRead();
    void onHeartbeat();
    void onConnectionTimeout();
    void onSocketError(QAbstractSocket::SocketError error);
    void attemptReconnect();

private:
    void startReconnect();
    void stopReconnect();

    QByteArray m_buffer;
    QTimer*    m_heartbeatTimer;   // 30s PING
    QTimer*    m_timeoutTimer;     // 90s 超时
    QTimer*    m_reconnectTimer;   // 指数退避重连
    uint32_t   m_seq = 0;

    // 重连参数
    QString    m_host;
    quint16    m_port = 0;
    int        m_reconnectAttempt = 0;
    static constexpr int MAX_RECONNECT_ATTEMPTS = 5;
    bool       m_reconnecting = false;
};
