#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QTimer>
#include "protocol.h"

/// Async TCP client for TinyWeChat protocol.
/// Emits signals on frame arrival, auto-reconnects, handles heartbeats.
class NetworkClient : public QObject
{
    Q_OBJECT
public:
    explicit NetworkClient(QObject* parent = nullptr);

    void connectToServer(const QString& host, quint16 port);
    void disconnect();
    bool isConnected() const;

    /// Send a REQUEST frame. seq is auto-incremented.
    int sendRequest(const QString& type, const QJsonObject& body);

    /// Send a NOTIFICATION frame.
    void sendNotification(const QString& type, const QJsonObject& body);

signals:
    void connected();
    void disconnected();
    void loginResult(int code, int userId, const QString& nickname);
    void messageReceived(int fromUid, const QString& fromNick,
                         int msgId, const QString& content, int timestamp);
    void groupMessageReceived(int groupId, int fromUid, const QString& fromNick,
                              int msgSeq, const QString& content, int timestamp);
    void responseReceived(const QString& type, int seq,
                          const QJsonObject& body);
    void notificationReceived(const QString& type,
                              const QJsonObject& body);
    void errorOccurred(const QString& message);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onHeartbeat();

private:
    void processFrame(FrameType ft, const QJsonObject& obj);

    QTcpSocket* socket_;
    QByteArray  recvBuffer_;
    QTimer*     heartbeatTimer_;
    int         seqCounter_ = 1;
};
