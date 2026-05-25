#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../network/WeChatSocket.h"

class LoginWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWidget(WeChatSocket* socket, QWidget* parent = nullptr);

signals:
    void loggedIn(int userId, const QString& nickname);
    void goRegister();   // 切换到注册页

private slots:
    void onLoginClicked();
    void onConnected();
    void onFrameReceived(const Frame& frame);
    void onConnectionFailed(const QString& reason);

private:
    void showStatus(const QString& text, const QString& bgColor, const QString& textColor);

    WeChatSocket* m_socket;
    QLineEdit*    m_usernameEdit;
    QLineEdit*    m_passwordEdit;
    QPushButton*  m_loginBtn;
    QPushButton*  m_loginTab;
    QPushButton*  m_registerTab;
    QLabel*       m_statusLabel = nullptr;
};
