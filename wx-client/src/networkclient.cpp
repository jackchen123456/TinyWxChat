#include "networkclient.h"
#include <QJsonDocument>

NetworkClient::NetworkClient(QObject* parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
    , heartbeatTimer_(new QTimer(this))
{
    connect(socket_, &QTcpSocket::readyRead,
            this, &NetworkClient::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected,
            this, &NetworkClient::onDisconnected);
    connect(heartbeatTimer_, &QTimer::timeout,
            this, &NetworkClient::onHeartbeat);
}

void NetworkClient::connectToServer(const QString& host, quint16 port)
{
    socket_->connectToHost(host, port);
    // Qt's socket will emit connected() when ready; we don't block.
    connect(socket_, &QTcpSocket::connected, this, [this]() {
        heartbeatTimer_->start(30000);  // 30s heartbeat
        emit connected();
    });
}

void NetworkClient::disconnect()
{
    heartbeatTimer_->stop();
    socket_->disconnectFromHost();
}

bool NetworkClient::isConnected() const
{
    return socket_->state() == QAbstractSocket::ConnectedState;
}

int NetworkClient::sendRequest(const QString& type, const QJsonObject& body)
{
    int seq = seqCounter_++;
    QByteArray frame = buildFrame(type, body, seq, FrameType::REQUEST);
    socket_->write(frame);
    return seq;
}

void NetworkClient::sendNotification(const QString& type, const QJsonObject& body)
{
    QByteArray frame = buildFrame(type, body, 0, FrameType::NOTIFICATION);
    socket_->write(frame);
}

// ── Slots ───────────────────────────────────────────────

void NetworkClient::onReadyRead()
{
    recvBuffer_.append(socket_->readAll());

    while (true) {
        FrameType ft;
        QJsonObject obj;
        QByteArray frame = parseFrame(recvBuffer_, ft, obj);
        if (frame.isEmpty()) break;  // incomplete
        processFrame(ft, obj);
    }
}

void NetworkClient::onDisconnected()
{
    heartbeatTimer_->stop();
    emit disconnected();
}

void NetworkClient::onHeartbeat()
{
    if (socket_->state() == QAbstractSocket::ConnectedState) {
        sendRequest(MsgType::PING, {});
    }
}

// ── Internal ─────────────────────────────────────────────

void NetworkClient::processFrame(FrameType ft, const QJsonObject& obj)
{
    QString type = obj.value("type").toString();
    QJsonObject body = obj.value("body").toObject();

    if (ft == FrameType::PINGPONG)
        return;  // ignore pong

    if (type == MsgType::LOGIN_RES) {
        emit loginResult(body.value("code").toInt(),
                         body.value("user_id").toInt(),
                         body.value("nickname").toString());
        return;
    }

    if (ft == FrameType::RESPONSE) {
        int seq = obj.value("seq").toInt();
        emit responseReceived(type, seq, body);
        return;
    }

    if (ft == FrameType::NOTIFICATION) {
        if (type == MsgType::RECV) {
            emit messageReceived(
                body.value("from_user_id").toInt(),
                body.value("from_nickname").toString(),
                body.value("msg_id").toInt(),
                body.value("content").toString(),
                body.value("timestamp").toInt());
        } else if (type == MsgType::GROUP_RECV) {
            emit groupMessageReceived(
                body.value("group_id").toInt(),
                body.value("from_user_id").toInt(),
                body.value("from_nickname").toString(),
                body.value("msg_seq").toInt(),
                body.value("content").toString(),
                body.value("timestamp").toInt());
        } else {
            emit notificationReceived(type, body);
        }
    }
}
