#include "RegisterWidget.h"
#include "../protocol/MessageBuilder.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QDebug>

RegisterWidget::RegisterWidget(WeChatSocket* socket, QWidget* parent)
    : QWidget(parent)
    , m_socket(socket)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(60, 60, 60, 60);

    auto* title = new QLabel("注册 TinyWeChat");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 20px; font-weight: bold; margin-bottom: 16px;");
    mainLayout->addWidget(title);

    auto* form = new QFormLayout();
    m_usernameEdit = new QLineEdit();
    m_usernameEdit->setPlaceholderText("用户名（最长32字节）");
    m_nicknameEdit = new QLineEdit();
    m_nicknameEdit->setPlaceholderText("昵称");
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setPlaceholderText("密码（至少6位）");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordConfirmEdit = new QLineEdit();
    m_passwordConfirmEdit->setPlaceholderText("确认密码");
    m_passwordConfirmEdit->setEchoMode(QLineEdit::Password);

    form->addRow("用户名：", m_usernameEdit);
    form->addRow("昵  称：", m_nicknameEdit);
    form->addRow("密  码：", m_passwordEdit);
    form->addRow("确认密码：", m_passwordConfirmEdit);
    mainLayout->addLayout(form);

    auto* btnLayout = new QHBoxLayout();
    m_backBtn = new QPushButton("返回登录");
    m_registerBtn = new QPushButton("注 册");
    m_registerBtn->setMinimumHeight(34);
    m_registerBtn->setStyleSheet("font-size: 15px;");
    btnLayout->addWidget(m_backBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_registerBtn);
    mainLayout->addLayout(btnLayout);

    m_statusLabel = new QLabel();
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #e74c3c; margin-top: 10px;");
    mainLayout->addWidget(m_statusLabel);

    mainLayout->addStretch();

    connect(m_registerBtn, &QPushButton::clicked, this, &RegisterWidget::onRegisterClicked);
    connect(m_backBtn, &QPushButton::clicked, this, &RegisterWidget::backToLogin);
    connect(m_socket, &WeChatSocket::frameReceived, this, &RegisterWidget::onFrameReceived);
    connect(m_socket, &QTcpSocket::connected, this, &RegisterWidget::onConnected);
    connect(m_socket, &WeChatSocket::connectionFailed, this, &RegisterWidget::onConnectionFailed);
}

void RegisterWidget::onRegisterClicked()
{
    QString username = m_usernameEdit->text().trimmed();
    QString nickname = m_nicknameEdit->text().trimmed();
    QString password = m_passwordEdit->text();
    QString confirm  = m_passwordConfirmEdit->text();

    if (username.isEmpty() || password.isEmpty() || nickname.isEmpty()) {
        m_statusLabel->setStyleSheet("color: #e74c3c; margin-top: 10px;");
        m_statusLabel->setText("所有字段都不能为空");
        return;
    }
    if (password != confirm) {
        m_statusLabel->setStyleSheet("color: #e74c3c; margin-top: 10px;");
        m_statusLabel->setText("两次输入的密码不一致");
        return;
    }
    if (password.size() < 6) {
        m_statusLabel->setStyleSheet("color: #e74c3c; margin-top: 10px;");
        m_statusLabel->setText("密码至少需要6位");
        return;
    }
    // 客户端校验用户名长度（§4 Phase 2: 402 = USERNAME_TOO_LONG，最大32字节UTF-8）
    if (username.toUtf8().size() > 32) {
        m_statusLabel->setStyleSheet("color: #e74c3c; margin-top: 10px;");
        m_statusLabel->setText("用户名超过 32 字节限制，请缩短用户名");
        return;
    }

    m_statusLabel->setStyleSheet("color: #555; margin-top: 10px;");
    m_statusLabel->setText("正在连接服务器...");
    m_registerBtn->setEnabled(false);

    m_socket->connectToServer("127.0.0.1", 9090);
}

void RegisterWidget::onConnected()
{
    m_statusLabel->setText("正在注册...");
    QJsonObject msg = MessageBuilder::buildRegister(
        m_usernameEdit->text().trimmed(),
        m_passwordEdit->text(),
        m_nicknameEdit->text().trimmed()
    );
    msg["seq"] = static_cast<int>(m_socket->nextSeq());
    m_socket->sendFrame(FrameType::REQUEST, msg);
}

void RegisterWidget::onFrameReceived(const Frame& frame)
{
    QString type = frame.payload.value("type").toString();

    if (type == "auth.register_res") {
        RegisterResponse r = MessageBuilder::parseRegisterResponse(frame.payload);
        if (r.ok) {
            m_statusLabel->setStyleSheet("color: #27ae60; margin-top: 10px;");
            m_statusLabel->setText(QString("注册成功！用户ID: %1，返回登录页即可登录").arg(r.userId));
            m_socket->close();
            m_registerBtn->setEnabled(true);
            emit registered(m_usernameEdit->text().trimmed());
        } else {
            m_statusLabel->setStyleSheet("color: #e74c3c; margin-top: 10px;");
            // §4 Phase 2: 根据错误码显示友好提示
            QString hint;
            switch (r.code) {
            case 200: hint = "请求格式错误，请检查所有字段是否填写"; break;
            case 400: hint = "该用户名已被注册，请换一个用户名"; break;
            case 402: hint = "用户名超过 32 字节限制，请缩短"; break;
            default:  hint = r.errorMsg.isEmpty() ? QString("注册失败 (code=%1)").arg(r.code)
                                                  : r.errorMsg; break;
            }
            m_statusLabel->setText(hint);
            m_registerBtn->setEnabled(true);
        }
    } else if (type == "error") {
        ErrorNotification e = MessageBuilder::parseError(frame.payload);
        QString hint;
        switch (e.code) {
        case 210: hint = "请先登录再注册（或连接已失效）"; break;
        default:  hint = e.message.isEmpty() ? QString("错误 (code=%1)").arg(e.code) : e.message; break;
        }
        m_statusLabel->setText(hint);
        m_registerBtn->setEnabled(true);
    }
}

void RegisterWidget::onConnectionFailed(const QString& reason)
{
    m_statusLabel->setStyleSheet("color: #e74c3c; margin-top: 10px;");
    m_statusLabel->setText(QString("连接失败：%1").arg(reason));
    m_registerBtn->setEnabled(true);
}
