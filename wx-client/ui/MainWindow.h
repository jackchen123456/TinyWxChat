#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include "LoginWidget.h"
#include "RegisterWidget.h"
#include "MainHubWidget.h"

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

private:
    enum Page { LOGIN = 0, REGISTER = 1, HUB = 2 };

    WeChatSocket*   m_socket;
    QStackedWidget* m_stack;

    LoginWidget*     m_loginWidget;
    RegisterWidget*  m_registerWidget;
    MainHubWidget*   m_hubWidget = nullptr;

    int     m_myUserId = 0;
    QString m_myNickname;
};
