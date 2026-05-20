#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include "LoginWidget.h"
#include "RegisterWidget.h"
#include "MainHubWidget.h"
#include "ChatWidget.h"
#include "GroupChatWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onLoggedIn(int userId, const QString& nickname);
    void onGoRegister();
    void onRegistered(const QString& username);
    void onBackToLogin();
    void onOpenChat(int userId, const QString& nickname);
    void onOpenGroupChat(int groupId, const QString& name);
    void onBackToHub();

private:
    enum Page { LOGIN = 0, REGISTER = 1, HUB = 2, CHAT = 3, GROUP_CHAT = 4 };

    WeChatSocket*   m_socket;
    QStackedWidget* m_stack;

    LoginWidget*     m_loginWidget;
    RegisterWidget*  m_registerWidget;
    MainHubWidget*   m_hubWidget = nullptr;
    ChatWidget*      m_chatWidget = nullptr;
    GroupChatWidget* m_groupChatWidget = nullptr;

    int     m_myUserId = 0;
    QString m_myNickname;
};
