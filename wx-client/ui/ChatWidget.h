#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../network/WeChatSocket.h"
#include "../protocol/MessageBuilder.h"

// 聊天页面：消息列表 + 输入框 + 发送按钮
class ChatWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChatWidget(WeChatSocket* socket, int myUserId,
                        const QString& myNickname, QWidget* parent = nullptr);

    // 拉取与指定用户的历史消息
    void pullHistory(int withUserId);

signals:
    // 用户主动断开（关闭窗口等）
    void disconnected();

private slots:
    void onSendClicked();
    void onFrameReceived(const Frame& frame);
    void onConnectionFailed(const QString& reason);

private:
    void appendMessage(bool isMine, const QString& sender, const QString& content);
    void appendSystem(const QString& text);

    WeChatSocket* m_socket;
    int           m_myUserId;
    QString       m_myNickname;

    QListWidget*  m_messageList;
    QLineEdit*    m_inputEdit;
    QPushButton*  m_sendBtn;
    QLineEdit*    m_targetEdit;   // 目标用户 ID
    QLabel*       m_statusLabel;
};
