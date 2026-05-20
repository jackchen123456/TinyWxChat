#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QDateTime>
#include "../network/WeChatSocket.h"
#include "../protocol/MessageBuilder.h"

class ChatWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChatWidget(WeChatSocket* socket, int myUserId,
                        const QString& myNickname, QWidget* parent = nullptr);

    void openConversation(int userId, const QString& nickname);
    void pullHistory();
    int targetUserId() const { return m_targetUserId; }

signals:
    void back();

private slots:
    void onSendClicked();
    void onFrameReceived(const Frame& frame);
    void onConnectionFailed(const QString& reason);

private:
    void appendMessage(bool isMine, const QString& sender, const QString& content,
                       const QString& extra = QString());
    void appendImage(bool isMine, const QString& sender, const QString& base64Data);
    void appendSystem(const QString& text);

    WeChatSocket* m_socket;
    int           m_myUserId;
    QString       m_myNickname;
    int           m_targetUserId = 0;
    QString       m_targetNickname;

    QLabel*       m_titleLabel;
    QListWidget*  m_messageList;
    QLineEdit*    m_inputEdit;
    QPushButton*  m_sendBtn;
    QPushButton*  m_backBtn;
};
