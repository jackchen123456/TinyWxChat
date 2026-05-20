#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../network/WeChatSocket.h"

// 注册页面：用户名 + 昵称 + 密码 + 注册按钮
class RegisterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RegisterWidget(WeChatSocket* socket, QWidget* parent = nullptr);

signals:
    void registered(const QString& username);
    void backToLogin();

private slots:
    void onRegisterClicked();
    void onConnected();
    void onFrameReceived(const Frame& frame);
    void onConnectionFailed(const QString& reason);

private:
    WeChatSocket* m_socket;
    QLineEdit*    m_usernameEdit;
    QLineEdit*    m_nicknameEdit;
    QLineEdit*    m_passwordEdit;
    QLineEdit*    m_passwordConfirmEdit;
    QPushButton*  m_registerBtn;
    QPushButton*  m_backBtn;
    QLabel*       m_statusLabel;
};
