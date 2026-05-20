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

class GroupChatWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GroupChatWidget(WeChatSocket* socket, int myUserId,
                             const QString& myNickname, QWidget* parent = nullptr);

    void openGroup(int groupId, const QString& name);

signals:
    void back();

private slots:
    void onSendClicked();
    void onFrameReceived(const Frame& frame);
    void onConnectionFailed(const QString& reason);

private:
    void appendMessage(const QString& sender, const QString& content,
                       const QString& extra = QString());
    void appendImage(const QString& sender, const QString& base64Data);
    void appendSystem(const QString& text);

    WeChatSocket* m_socket;
    int           m_myUserId;
    QString       m_myNickname;
    int           m_groupId = 0;
    QString       m_groupName;

    QLabel*       m_titleLabel;
    QListWidget*  m_messageList;
    QLineEdit*    m_inputEdit;
    QPushButton*  m_sendBtn;
    QPushButton*  m_backBtn;
};
