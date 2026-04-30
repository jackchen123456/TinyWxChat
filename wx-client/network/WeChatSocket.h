#pragma once

#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include "FrameCodec.h"

// 封装 TCP 长连接、帧收发、心跳与超时管理
// 用法：
//   1. connectToServer(host, port) → 等待 connected 信号
//   2. 通过 frameReceived 信号接收帧（PING/PONG 已内部消化）
//   3. sendFrame() 发送帧
//   4. connectionFailed 信号处理断开/超时
class WeChatSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit WeChatSocket(QObject* parent = nullptr);

    // 连接到服务端（异步）
    void connectToServer(const QString& host, quint16 port);

    // 发送一帧；返回 true 表示写入成功（不代表对端收到）
    bool sendFrame(FrameType type, const QJsonObject& body);

    // 获取并递增消息序号（用于请求的 seq 字段）
    uint32_t nextSeq() { return m_seq++; }

    // 是否已连接（TCP 层面）
    bool isConnected() const { return state() == QAbstractSocket::ConnectedState; }

signals:
    // 收到一个完整帧（PING/PONG 已被内部过滤，不会触发此信号）
    void frameReceived(const Frame& frame);

    // 连接失败或超时断开
    void connectionFailed(const QString& reason);

private slots:
    void onReadyRead();
    void onHeartbeat();
    void onConnectionTimeout();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    QByteArray m_buffer;           // 接收缓冲区（可能包含不完整的帧）
    QTimer*    m_heartbeatTimer;   // 30s 发送 PING
    QTimer*    m_timeoutTimer;     // 90s 无数据则超时
    uint32_t   m_seq = 0;          // 请求序号
};
