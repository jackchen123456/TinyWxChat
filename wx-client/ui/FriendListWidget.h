#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QDialog>
#include <QLineEdit>
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

class FriendListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FriendListWidget(WeChatSocket* socket, int myUserId, QWidget* parent = nullptr);
    void refreshFriends();
    void refreshPending();

signals:
    void startChat(int userId, const QString& nickname);
    // 好友列表加载完成（用于同步会话列表）
    void friendsLoaded(const QList<QPair<int, QString>>& friends);

private slots:
    void onAddFriend();
    void onFrameReceived(const Frame& frame);

private:
    WeChatSocket* m_socket;
    int           m_myUserId;
    QListWidget*  m_friendList;
    QListWidget*  m_pendingList;
    QPushButton*  m_addBtn;
    QLabel*       m_friendCount;
    QLabel*       m_pendingLabel;
};
