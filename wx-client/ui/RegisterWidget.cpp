#include "RegisterWidget.h"
#include "../protocol/MessageBuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QGraphicsDropShadowEffect>
#include <QDebug>

RegisterWidget::RegisterWidget(WeChatSocket* socket, QWidget* parent)
    : QWidget(parent)
    , m_socket(socket)
{
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor("#f4f7f6"));
    setAutoFillBackground(true);
    setPalette(pal);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 24, 24, 24);
    rootLayout->setAlignment(Qt::AlignCenter);

    auto* card = new QWidget();
    card->setFixedWidth(440);
    card->setStyleSheet("QWidget { background: white; border-radius: 12px; }");

    auto* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 6);
    shadow->setColor(QColor(24, 32, 38, 40));
    card->setGraphicsEffect(shadow);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(48, 44, 48, 40);
    cardLayout->setSpacing(0);

    // Logo
    auto* logoRow = new QHBoxLayout();
    logoRow->addStretch();
    auto* logoLabel = new QLabel("💬");
    logoLabel->setFixedSize(42, 42);
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setStyleSheet("font-size: 24px; background: #27c277; border-radius: 8px; color: white;");
    logoRow->addWidget(logoLabel);
    logoRow->addSpacing(10);
    auto* logoText = new QLabel("TinyWeChat");
    logoText->setStyleSheet("font-size: 20px; font-weight: bold; color: #1f2a32; background: transparent;");
    logoRow->addWidget(logoText);
    logoRow->addStretch();
    cardLayout->addLayout(logoRow);
    cardLayout->addSpacing(22);

    auto* eyebrow = new QLabel("创建账号");
    eyebrow->setStyleSheet("font-size: 13px; font-weight: 800; color: #158a57; background: transparent;");
    eyebrow->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(eyebrow);
    cardLayout->addSpacing(6);

    auto* formTitle = new QLabel("注册新的账号");
    formTitle->setStyleSheet("font-size: 28px; font-weight: bold; color: #1f2a32; background: transparent;");
    formTitle->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(formTitle);
    cardLayout->addSpacing(4);

    auto* formSubtitle = new QLabel("设置用户名、昵称和密码，开始本地聊天。");
    formSubtitle->setStyleSheet("font-size: 14px; color: #7a8790; background: transparent;");
    formSubtitle->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(formSubtitle);
    cardLayout->addSpacing(24);

    // Tab
    auto* tabBg = new QWidget();
    tabBg->setStyleSheet("QWidget { background: #f7faf8; border: 1px solid #dde3e8; border-radius: 8px; }");
    auto* tabBgLayout = new QHBoxLayout(tabBg);
    tabBgLayout->setContentsMargins(5, 5, 5, 5);
    tabBgLayout->setSpacing(6);

    m_loginTab = new QPushButton("登录");
    m_loginTab->setCheckable(true);
    m_loginTab->setFixedHeight(36);
    m_loginTab->setCursor(Qt::PointingHandCursor);
    m_loginTab->setStyleSheet(
        "QPushButton { border: none; border-radius: 6px; font-size: 14px;"
        "  color: #7a8790; background: transparent; }"
        "QPushButton:hover { color: #202a31; }"
    );

    m_registerTab = new QPushButton("注册");
    m_registerTab->setCheckable(true);
    m_registerTab->setChecked(true);
    m_registerTab->setFixedHeight(36);
    m_registerTab->setCursor(Qt::PointingHandCursor);
    m_registerTab->setStyleSheet(
        "QPushButton { border: none; border-radius: 6px; font-size: 14px; font-weight: 800;"
        "  color: #202a31; background: white; }"
    );

    tabBgLayout->addWidget(m_loginTab);
    tabBgLayout->addWidget(m_registerTab);
    cardLayout->addWidget(tabBg);
    cardLayout->addSpacing(22);

    // 用户名 + 昵称
    auto* row1 = new QHBoxLayout();
    row1->setSpacing(14);

    auto* userCol = new QVBoxLayout();
    userCol->setSpacing(6);
    auto* userLabel = new QLabel("用户名");
    userLabel->setStyleSheet("font-size: 13px; font-weight: 800; color: #33414a; background: transparent;");
    userCol->addWidget(userLabel);
    m_usernameEdit = new QLineEdit();
    m_usernameEdit->setPlaceholderText("不超过 32 字节");
    m_usernameEdit->setMinimumHeight(44);
    m_usernameEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dde3e8; border-radius: 8px;"
        "  padding: 0 14px; background: #fbfcfb; font-size: 15px; color: #202a31; }"
        "QLineEdit:focus { border-color: rgba(39,194,119,0.6); background: white; }"
    );
    userCol->addWidget(m_usernameEdit);
    row1->addLayout(userCol, 1);

    auto* nickCol = new QVBoxLayout();
    nickCol->setSpacing(6);
    auto* nickLabel = new QLabel("昵称");
    nickLabel->setStyleSheet("font-size: 13px; font-weight: 800; color: #33414a; background: transparent;");
    nickCol->addWidget(nickLabel);
    m_nicknameEdit = new QLineEdit();
    m_nicknameEdit->setPlaceholderText("例如 Jack");
    m_nicknameEdit->setMinimumHeight(44);
    m_nicknameEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dde3e8; border-radius: 8px;"
        "  padding: 0 14px; background: #fbfcfb; font-size: 15px; color: #202a31; }"
        "QLineEdit:focus { border-color: rgba(39,194,119,0.6); background: white; }"
    );
    nickCol->addWidget(m_nicknameEdit);
    row1->addLayout(nickCol, 1);

    cardLayout->addLayout(row1);
    cardLayout->addSpacing(16);

    // 密码
    auto* passLabel = new QLabel("密码");
    passLabel->setStyleSheet("font-size: 13px; font-weight: 800; color: #33414a; background: transparent;");
    cardLayout->addWidget(passLabel);
    cardLayout->addSpacing(6);

    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setPlaceholderText("至少 6 位");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setMinimumHeight(44);
    m_passwordEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dde3e8; border-radius: 8px;"
        "  padding: 0 14px; background: #fbfcfb; font-size: 15px; color: #202a31; }"
        "QLineEdit:focus { border-color: rgba(39,194,119,0.6); background: white; }"
    );
    cardLayout->addWidget(m_passwordEdit);
    cardLayout->addSpacing(16);

    // 确认密码
    auto* confirmLabel = new QLabel("确认密码");
    confirmLabel->setStyleSheet("font-size: 13px; font-weight: 800; color: #33414a; background: transparent;");
    cardLayout->addWidget(confirmLabel);
    cardLayout->addSpacing(6);

    m_passwordConfirmEdit = new QLineEdit();
    m_passwordConfirmEdit->setPlaceholderText("再次输入密码");
    m_passwordConfirmEdit->setEchoMode(QLineEdit::Password);
    m_passwordConfirmEdit->setMinimumHeight(44);
    m_passwordConfirmEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dde3e8; border-radius: 8px;"
        "  padding: 0 14px; background: #fbfcfb; font-size: 15px; color: #202a31; }"
        "QLineEdit:focus { border-color: rgba(39,194,119,0.6); background: white; }"
    );
    cardLayout->addWidget(m_passwordConfirmEdit);
    cardLayout->addSpacing(14);

    // 协议
    auto* agreementCheck = new QCheckBox("同意本地学习项目账号规则");
    agreementCheck->setStyleSheet(
        "QCheckBox { font-size: 13px; color: #7a8790; spacing: 8px; background: transparent; }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
    );
    cardLayout->addWidget(agreementCheck);
    cardLayout->addSpacing(18);

    // 注册按钮
    m_registerBtn = new QPushButton("创建账号 +");
    m_registerBtn->setMinimumHeight(48);
    m_registerBtn->setCursor(Qt::PointingHandCursor);
    m_registerBtn->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #27c277, stop:1 #3c82e6);"
        "  color: white; border: none; border-radius: 8px;"
        "  font-size: 16px; font-weight: bold;"
        "}"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1da865, stop:1 #2a6bc4); }"
        "QPushButton:pressed { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #158a57, stop:1 #2060b0); }"
        "QPushButton:disabled { background: #b0c4b8; }"
    );
    cardLayout->addWidget(m_registerBtn);

    // 动态 status label
    m_statusLabel = nullptr;

    rootLayout->addWidget(card);

    // 信号
    connect(m_registerBtn, &QPushButton::clicked, this, &RegisterWidget::onRegisterClicked);
    connect(m_loginTab, &QPushButton::clicked, this, &RegisterWidget::backToLogin);
    connect(m_socket, &WeChatSocket::frameReceived, this, &RegisterWidget::onFrameReceived);
    connect(m_socket, &QTcpSocket::connected, this, &RegisterWidget::onConnected);
    connect(m_socket, &WeChatSocket::connectionFailed, this, &RegisterWidget::onConnectionFailed);
}

void RegisterWidget::showStatus(const QString& text, const QString& bgColor, const QString& textColor)
{
    if (!m_statusLabel) {
        m_statusLabel = new QLabel();
        m_statusLabel->setWordWrap(true);
        auto* card = qobject_cast<QWidget*>(m_registerBtn->parent());
        if (card) {
            auto* lay = qobject_cast<QVBoxLayout*>(card->layout());
            if (lay) {
                int idx = lay->indexOf(m_registerBtn);
                lay->insertWidget(idx + 1, m_statusLabel);
            }
        }
    }
    if (bgColor.isEmpty()) {
        m_statusLabel->setStyleSheet(
            QString("font-size: 13px; color: %1; background: transparent; padding: 0; margin-top: 6px;").arg(textColor));
    } else {
        m_statusLabel->setStyleSheet(
            QString("font-size: 13px; color: %1; background: %2; border-radius: 6px; padding: 8px 12px; margin-top: 6px;")
                .arg(textColor, bgColor));
    }
    m_statusLabel->setText(text);
    m_statusLabel->show();
}

void RegisterWidget::onRegisterClicked()
{
    QString username = m_usernameEdit->text().trimmed();
    QString nickname = m_nicknameEdit->text().trimmed();
    QString password = m_passwordEdit->text();
    QString confirm  = m_passwordConfirmEdit->text();

    if (username.isEmpty() || password.isEmpty() || nickname.isEmpty()) {
        showStatus("用户名和昵称不能为空。", "#fff0f0", "#c93d3d");
        return;
    }
    if (username.toUtf8().size() > 32) {
        showStatus("用户名不能超过 32 字节。", "#fff0f0", "#c93d3d");
        return;
    }
    if (password.size() < 6) {
        showStatus("密码至少需要 6 位。", "#fff0f0", "#c93d3d");
        return;
    }
    if (password != confirm) {
        showStatus("两次输入的密码不一致。", "#fff0f0", "#c93d3d");
        return;
    }

    showStatus("正在连接服务器...", "", "#7a8790");
    m_registerBtn->setEnabled(false);
    m_socket->connectToServer("127.0.0.1", 9090);
}

void RegisterWidget::onConnected()
{
    showStatus("正在注册...", "", "#7a8790");
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

    if (type == MsgType::REGISTER_RES) {
        RegisterResponse r = MessageBuilder::parseRegisterResponse(frame.payload);
        if (r.ok) {
            showStatus(QString("注册成功！用户ID: %1，返回登录页即可登录").arg(r.userId), "#e7f8ef", "#116544");
            m_socket->close();
            m_registerBtn->setEnabled(true);
            emit registered(m_usernameEdit->text().trimmed());
        } else {
            QString hint;
            switch (r.code) {
            case 200: hint = "请求格式错误，请检查所有字段是否填写"; break;
            case 400: hint = "该用户名已被注册，请换一个用户名"; break;
            case 402: hint = "用户名超过 32 字节限制，请缩短"; break;
            default:  hint = r.errorMsg.isEmpty() ? QString("注册失败 (code=%1)").arg(r.code) : r.errorMsg; break;
            }
            showStatus(hint, "#fff0f0", "#c93d3d");
            m_registerBtn->setEnabled(true);
        }
    } else if (type == MsgType::ERROR) {
        ErrorNotification e = MessageBuilder::parseError(frame.payload);
        QString hint;
        switch (e.code) {
        case 210: hint = "请先登录再注册（或连接已失效）"; break;
        default:  hint = e.message.isEmpty() ? QString("错误 (code=%1)").arg(e.code) : e.message; break;
        }
        showStatus(hint, "#fff0f0", "#c93d3d");
        m_registerBtn->setEnabled(true);
    }
}

void RegisterWidget::onConnectionFailed(const QString& reason)
{
    showStatus(QString("连接失败：%1").arg(reason), "#fff0f0", "#c93d3d");
    m_registerBtn->setEnabled(true);
}
