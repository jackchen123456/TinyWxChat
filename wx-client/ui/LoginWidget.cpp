#include "LoginWidget.h"
#include "../protocol/MessageBuilder.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDebug>

LoginWidget::LoginWidget(WeChatSocket* socket, QWidget* parent)
    : QWidget(parent)
    , m_socket(socket)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(60, 80, 60, 80);

    auto* title = new QLabel("TinyWeChat 登录");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 22px; font-weight: bold; margin-bottom: 20px;");
    mainLayout->addWidget(title);

    auto* form = new QFormLayout();
    m_usernameEdit = new QLineEdit();
    m_usernameEdit->setPlaceholderText("请输入用户名");
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setPlaceholderText("请输入密码");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    form->addRow("用户名：", m_usernameEdit);
    form->addRow("密  码：", m_passwordEdit);
    mainLayout->addLayout(form);

    m_loginBtn = new QPushButton("登 录");
    m_loginBtn->setMinimumHeight(36);
    m_loginBtn->setStyleSheet("font-size: 15px;");
    mainLayout->addWidget(m_loginBtn);

    m_statusLabel = new QLabel();
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #e74c3c; margin-top: 10px;");
    mainLayout->addWidget(m_statusLabel);

    auto* registerLink = new QPushButton("还没有账号？立即注册");
    registerLink->setFlat(true);
    registerLink->setStyleSheet("color: #3498db; border: none; font-size: 12px;");
    mainLayout->addWidget(registerLink);
    connect(registerLink, &QPushButton::clicked, this, &LoginWidget::goRegister);

    mainLayout->addStretch();

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginWidget::onLoginClicked);
    connect(m_socket, &WeChatSocket::frameReceived, this, &LoginWidget::onFrameReceived);
    connect(m_socket, &QTcpSocket::connected, this, &LoginWidget::onConnected);
    connect(m_socket, &WeChatSocket::connectionFailed, this, &LoginWidget::onConnectionFailed);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginWidget::onLoginClicked);
}

void LoginWidget::onLoginClicked()
{
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText("用户名和密码不能为空");
        return;
    }

    m_statusLabel->setText("正在连接服务器...");
    m_loginBtn->setEnabled(false);
    m_socket->connectToServer("127.0.0.1", 9090);
}

void LoginWidget::onConnected()
{
    qDebug() << "[LoginWidget] TCP connected, sending login";
    m_statusLabel->setText("正在登录...");

    QJsonObject loginMsg = MessageBuilder::buildLogin(
        m_usernameEdit->text().trimmed(),
        m_passwordEdit->text()
    );
    loginMsg["seq"] = static_cast<int>(m_socket->nextSeq());
    m_socket->sendFrame(FrameType::REQUEST, loginMsg);
}

void LoginWidget::onFrameReceived(const Frame& frame)
{
    QString type = frame.payload.value("type").toString();

    if (type == "auth.login_res") {
        LoginResponse r = MessageBuilder::parseLoginResponse(frame.payload);
        if (r.ok) {
            m_statusLabel->setStyleSheet("color: #27ae60; margin-top: 10px;");
            m_statusLabel->setText("登录成功！");
            emit loggedIn(r.userId, r.nickname);
        } else {
            m_statusLabel->setStyleSheet("color: #e74c3c; margin-top: 10px;");
            // §8: 登录失败时显示具体错误原因（如"用户名或密码错误"）
            QString hint;
            switch (r.code) {
            case 300: hint = "用户不存在，请检查用户名"; break;
            case 401: hint = "密码错误，请重新输入"; break;
            case 200: hint = "请求格式错误，请检查用户名和密码"; break;
            case 210: hint = "未授权，请重新登录"; break;
            default:
                hint = r.errorMsg.isEmpty()
                    ? QString("登录失败 (code=%1)").arg(r.code)
                    : r.errorMsg;
                break;
            }
            m_statusLabel->setText(hint);
            m_loginBtn->setEnabled(true);
        }
    } else if (type == "error") {
        ErrorNotification e = MessageBuilder::parseError(frame.payload);
        QString hint;
        switch (e.code) {
        case 210: hint = "未登录，请检查连接状态"; break;
        default:  hint = e.message.isEmpty() ? QString("错误 (code=%1)").arg(e.code) : e.message; break;
        }
        m_statusLabel->setText(hint);
        m_loginBtn->setEnabled(true);
    }
}

void LoginWidget::onConnectionFailed(const QString& reason)
{
    qWarning() << "[LoginWidget] Connection failed:" << reason;
    m_statusLabel->setStyleSheet("color: #e74c3c; margin-top: 10px;");
    m_statusLabel->setText(QString("连接失败：%1").arg(reason));
    m_loginBtn->setEnabled(true);
}
