#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../network/WeChatSocket.h"

// 登录页面：用户名 + 密码 + 登录按钮 + 状态提示
class LoginWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWidget(WeChatSocket* socket, QWidget* parent = nullptr);

signals:
    // 登录成功时发出
    void loggedIn(int userId, const QString& nickname);

private slots:
    void onLoginClicked();
    void onConnected();
    void onFrameReceived(const Frame& frame);
    void onConnectionFailed(const QString& reason);

private:
    WeChatSocket* m_socket;
    QLineEdit*    m_usernameEdit;
    QLineEdit*    m_passwordEdit;
    QPushButton*  m_loginBtn;
    QLabel*       m_statusLabel;
    bool          m_connecting = false;
};
