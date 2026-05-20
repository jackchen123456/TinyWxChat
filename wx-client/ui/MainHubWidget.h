#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include "ConversationListWidget.h"
#include "FriendListWidget.h"
#include "GroupListWidget.h"
#include "AvatarLabel.h"

class MainHubWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainHubWidget(WeChatSocket* socket, int myUserId,
                           const QString& myNickname, QWidget* parent = nullptr);

    ConversationListWidget* conversationList() const { return m_convList; }

signals:
    void openChat(int userId, const QString& nickname);
    void openGroupChat(int groupId, const QString& name);

private slots:
    void switchTab(int index);
    void onChangeAvatar();
    void onFriendsLoaded(const QList<QPair<int, QString>>& friends);

private:
    WeChatSocket*           m_socket;
    int                     m_myUserId;
    QString                 m_myNickname;

    QStackedWidget*         m_stack;
    ConversationListWidget* m_convList;
    FriendListWidget*       m_friendList;
    GroupListWidget*        m_groupList;
    AvatarLabel*            m_myAvatar;
    QColor                  m_avatarColor;

    QList<QPushButton*>     m_tabBtns;
    void setActiveTab(int index);
};
