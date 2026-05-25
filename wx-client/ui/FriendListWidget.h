#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QDialog>
#include <QList>
#include "../network/WeChatSocket.h"
#include "../protocol/MessageBuilder.h"

class FriendListWidget;

class AddFriendDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddFriendDialog(WeChatSocket* socket, QWidget* parent = nullptr);
signals:
    void requestSent();
private slots:
    void onSendClicked();
private:
    WeChatSocket* m_socket;
    QLineEdit*    m_userIdEdit;
    QLineEdit*    m_messageEdit;
    QLabel*       m_statusLabel;
};

// 精简版：只负责显示好友列表和待处理申请，不含标题/搜索/按钮等外部 chrome
class FriendListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FriendListWidget(WeChatSocket* socket, int myUserId, QWidget* parent = nullptr);
    void refreshFriends();
    void refreshPending();

signals:
    void startChat(int userId, const QString& nickname);
    void friendsLoaded(const QList<QPair<int, QString>>& friends);

private slots:
    void onFrameReceived(const Frame& frame);

private:
    WeChatSocket* m_socket;
    int           m_myUserId;
    QListWidget*  m_friendList;
    QListWidget*  m_pendingList;
};
