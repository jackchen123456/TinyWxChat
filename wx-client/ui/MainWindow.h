#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include "LoginWidget.h"
#include "ChatWidget.h"

// 主窗口：管理登录页 ↔ 聊天页的切换
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onLoggedIn(int userId, const QString& nickname);

private:
    WeChatSocket*   m_socket;      // 整个生命周期拥有 socket
    QStackedWidget* m_stack;
    LoginWidget*    m_loginWidget;
    ChatWidget*     m_chatWidget = nullptr;
};
