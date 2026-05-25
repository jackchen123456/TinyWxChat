#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include "ConversationListWidget.h"
#include "FriendListWidget.h"
#include "GroupListWidget.h"
#include "ChatWidget.h"
#include "GroupChatWidget.h"
#include "AvatarLabel.h"

class MainHubWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainHubWidget(WeChatSocket* socket, int myUserId,
                           const QString& myNickname, QWidget* parent = nullptr);

    ConversationListWidget* conversationList() const { return m_convList; }

private slots:
    void switchSidebar(int index);
    void onChangeAvatar();
    void onFriendsLoaded(const QList<QPair<int, QString>>& friends);
    void onOpenChat(int userId, const QString& nickname);
    void onOpenGroupChat(int groupId, const QString& name);

private:
    WeChatSocket*           m_socket;
    int                     m_myUserId;
    QString                 m_myNickname;

    // ── 标题栏 ──
    QLabel*                 m_connectionPill;

    // ── 中间面板：列表区 ──
    QStackedWidget*         m_middleStack;
    ConversationListWidget* m_convList;
    FriendListWidget*       m_friendList;
    GroupListWidget*        m_groupList;
    QLabel*                 m_sideTitle;
    QLabel*                 m_sideCount;
    QPushButton*            m_sideAddBtn;

    // ── 右侧面板：聊天区 ──
    QStackedWidget*         m_rightStack;
    ChatWidget*             m_chatWidget;
    GroupChatWidget*        m_groupChatWidget;
    QWidget*                m_emptyChatWidget;
    QWidget*                m_composerWidget;

    // ── 左侧边栏 ──
    AvatarLabel*            m_myAvatar;
    QColor                  m_avatarColor;
    QList<QPushButton*>     m_sidebarBtns;
    void setActiveSidebar(int index);
};
