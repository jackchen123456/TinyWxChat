#include "LoginWidget.h"
#include "../protocol/MessageBuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QGraphicsDropShadowEffect>
#include <QDebug>

LoginWidget::LoginWidget(WeChatSocket* socket, QWidget* parent)
    : QWidget(parent)
    , m_socket(socket)
{
    // 页面背景
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor("#f4f7f6"));
    setAutoFillBackground(true);
    setPalette(pal);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 24, 24, 24);
    rootLayout->setAlignment(Qt::AlignCenter);

    // ══════════════════════════════════════════
    // 居中卡片（无边框，用阴影定义轮廓）
    // ══════════════════════════════════════════
    auto* card = new QWidget();
    card->setFixedWidth(440);
    card->setStyleSheet("QWidget { background: white; border-radius: 12px; }");

    // 阴影效果
    auto* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 6);
    shadow->setColor(QColor(24, 32, 38, 40));
    card->setGraphicsEffect(shadow);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(48, 44, 48, 40);
    cardLayout->setSpacing(0);

    // Logo + TinyWeChat
    auto* logoRow = new QHBoxLayout();
    logoRow->addStretch();
    auto* logoLabel = new QLabel("💬");
    logoLabel->setFixedSize(42, 42);
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setStyleSheet(
        "font-size: 24px; background: #27c277; border-radius: 8px; color: white;"
    );
    logoRow->addWidget(logoLabel);
    logoRow->addSpacing(10);
    auto* logoText = new QLabel("TinyWeChat");
    logoText->setStyleSheet("font-size: 20px; font-weight: bold; color: #1f2a32; background: transparent;");
    logoRow->addWidget(logoText);
    logoRow->addStretch();
    cardLayout->addLayout(logoRow);
    cardLayout->addSpacing(22);

    // 欢迎回来
    auto* eyebrow = new QLabel("欢迎回来");
    eyebrow->setStyleSheet("font-size: 13px; font-weight: 800; color: #158a57; background: transparent;");
    eyebrow->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(eyebrow);
    cardLayout->addSpacing(6);

    auto* formTitle = new QLabel("登录你的账号");
    formTitle->setStyleSheet("font-size: 28px; font-weight: bold; color: #1f2a32; background: transparent;");
    formTitle->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(formTitle);
    cardLayout->addSpacing(4);

    auto* formSubtitle = new QLabel("输入用户名和密码，进入 TinyWeChat。");
    formSubtitle->setStyleSheet("font-size: 14px; color: #7a8790; background: transparent;");
    formSubtitle->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(formSubtitle);
    cardLayout->addSpacing(24);

    // ── 登录/注册 Tab ──
    auto* tabBg = new QWidget();
    tabBg->setStyleSheet(
        "QWidget { background: #f7faf8; border: 1px solid #dde3e8; border-radius: 8px; }"
    );
    auto* tabBgLayout = new QHBoxLayout(tabBg);
    tabBgLayout->setContentsMargins(5, 5, 5, 5);
    tabBgLayout->setSpacing(6);

    m_loginTab = new QPushButton("登录");
    m_loginTab->setCheckable(true);
    m_loginTab->setChecked(true);
    m_loginTab->setFixedHeight(36);
    m_loginTab->setCursor(Qt::PointingHandCursor);
    m_loginTab->setStyleSheet(
        "QPushButton { border: none; border-radius: 6px; font-size: 14px; font-weight: 800;"
        "  color: #202a31; background: white; }"
    );

    m_registerTab = new QPushButton("注册");
    m_registerTab->setCheckable(true);
    m_registerTab->setFixedHeight(36);
    m_registerTab->setCursor(Qt::PointingHandCursor);
    m_registerTab->setStyleSheet(
        "QPushButton { border: none; border-radius: 6px; font-size: 14px;"
        "  color: #7a8790; background: transparent; }"
        "QPushButton:hover { color: #202a31; }"
    );

    tabBgLayout->addWidget(m_loginTab);
    tabBgLayout->addWidget(m_registerTab);
    cardLayout->addWidget(tabBg);
    cardLayout->addSpacing(22);

    // ── 用户名 ──
    auto* userLabel = new QLabel("用户名");
    userLabel->setStyleSheet("font-size: 13px; font-weight: 800; color: #33414a; background: transparent;");
    cardLayout->addWidget(userLabel);
    cardLayout->addSpacing(6);

    m_usernameEdit = new QLineEdit();
    m_usernameEdit->setPlaceholderText("alice");
    m_usernameEdit->setMinimumHeight(46);
    m_usernameEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dde3e8; border-radius: 8px;"
        "  padding: 0 14px; background: #fbfcfb; font-size: 15px; color: #202a31; }"
        "QLineEdit:focus { border-color: rgba(39,194,119,0.6); background: white; }"
    );
    cardLayout->addWidget(m_usernameEdit);
    cardLayout->addSpacing(16);

    // ── 密码 ──
    auto* passLabel = new QLabel("密码");
    passLabel->setStyleSheet("font-size: 13px; font-weight: 800; color: #33414a; background: transparent;");
    cardLayout->addWidget(passLabel);
    cardLayout->addSpacing(6);

    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setPlaceholderText("123456");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setMinimumHeight(46);
    m_passwordEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dde3e8; border-radius: 8px;"
        "  padding: 0 14px; background: #fbfcfb; font-size: 15px; color: #202a31; }"
        "QLineEdit:focus { border-color: rgba(39,194,119,0.6); background: white; }"
    );
    cardLayout->addWidget(m_passwordEdit);
    cardLayout->addSpacing(14);

    // ── 记住登录 + 忘记密码 ──
    auto* optRow = new QHBoxLayout();
    auto* rememberCheck = new QCheckBox("记住登录状态");
    rememberCheck->setChecked(true);
    rememberCheck->setStyleSheet(
        "QCheckBox { font-size: 13px; color: #7a8790; spacing: 8px; background: transparent; }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
    );
    optRow->addWidget(rememberCheck);
    optRow->addStretch();
    auto* forgotLink = new QPushButton("忘记密码？");
    forgotLink->setFlat(true);
    forgotLink->setCursor(Qt::PointingHandCursor);
    forgotLink->setStyleSheet(
        "QPushButton { border: none; color: #3c82e6; font-size: 13px; font-weight: 800;"
        "  background: transparent; padding: 0; }"
        "QPushButton:hover { color: #2a6bc4; }"
    );
    optRow->addWidget(forgotLink);
    cardLayout->addLayout(optRow);
    cardLayout->addSpacing(18);

    // ── 登录按钮（绿蓝渐变）──
    m_loginBtn = new QPushButton("登录 →");
    m_loginBtn->setMinimumHeight(48);
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    m_loginBtn->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #27c277, stop:1 #3c82e6);"
        "  color: white; border: none; border-radius: 8px;"
        "  font-size: 16px; font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #1da865, stop:1 #2a6bc4);"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #158a57, stop:1 #2060b0);"
        "}"
        "QPushButton:disabled { background: #b0c4b8; }"
    );
    cardLayout->addWidget(m_loginBtn);

    rootLayout->addWidget(card);

    // ── 信号 ──
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginWidget::onLoginClicked);
    connect(m_socket, &WeChatSocket::frameReceived, this, &LoginWidget::onFrameReceived);
    connect(m_socket, &QTcpSocket::connected, this, &LoginWidget::onConnected);
    connect(m_socket, &WeChatSocket::connectionFailed, this, &LoginWidget::onConnectionFailed);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginWidget::onLoginClicked);
    connect(m_registerTab, &QPushButton::clicked, this, &LoginWidget::goRegister);

    // 初始隐藏 statusLabel（仅在出错时显示）
    m_statusLabel = nullptr;
}

void LoginWidget::showStatus(const QString& text, const QString& bgColor, const QString& textColor)
{
    if (!m_statusLabel) {
        // 动态创建，插入到登录按钮后面
        m_statusLabel = new QLabel();
        m_statusLabel->setWordWrap(true);
        // 找到卡片 layout，在按钮后插入
        auto* card = qobject_cast<QWidget*>(m_loginBtn->parent());
        if (card) {
            auto* lay = qobject_cast<QVBoxLayout*>(card->layout());
            if (lay) {
                int idx = lay->indexOf(m_loginBtn);
                lay->insertWidget(idx + 1, m_statusLabel);
            }
        }
    }
    if (bgColor.isEmpty()) {
        m_statusLabel->setStyleSheet(QString("font-size: 13px; color: %1; background: transparent; padding: 0; margin-top: 6px;").arg(textColor));
    } else {
        m_statusLabel->setStyleSheet(
            QString("font-size: 13px; color: %1; background: %2; border-radius: 6px; padding: 8px 12px; margin-top: 6px;")
                .arg(textColor, bgColor)
        );
    }
    m_statusLabel->setText(text);
    m_statusLabel->show();
}

void LoginWidget::onLoginClicked()
{
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        showStatus("请输入用户名和至少 6 位密码。", "#fff0f0", "#c93d3d");
        return;
    }

    showStatus("正在连接服务器...", "", "#7a8790");
    m_loginBtn->setEnabled(false);
    m_socket->connectToServer("127.0.0.1", 9090);
}

void LoginWidget::onConnected()
{
    qDebug() << "[LoginWidget] TCP connected, sending login";
    showStatus("正在登录...", "", "#7a8790");

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

    if (type == MsgType::LOGIN_RES) {
        LoginResponse r = MessageBuilder::parseLoginResponse(frame.payload);
        if (r.ok) {
            showStatus("登录成功！", "#e7f8ef", "#116544");
            emit loggedIn(r.userId, r.nickname);
        } else {
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
            showStatus(hint, "#fff0f0", "#c93d3d");
            m_loginBtn->setEnabled(true);
        }
    } else if (type == MsgType::ERROR) {
        ErrorNotification e = MessageBuilder::parseError(frame.payload);
        QString hint;
        switch (e.code) {
        case 210: hint = "未登录，请检查连接状态"; break;
        default:  hint = e.message.isEmpty() ? QString("错误 (code=%1)").arg(e.code) : e.message; break;
        }
        showStatus(hint, "#fff0f0", "#c93d3d");
        m_loginBtn->setEnabled(true);
    }
}

void LoginWidget::onConnectionFailed(const QString& reason)
{
    qWarning() << "[LoginWidget] Connection failed:" << reason;
    showStatus(QString("连接失败：%1").arg(reason), "#fff0f0", "#c93d3d");
    m_loginBtn->setEnabled(true);
}
