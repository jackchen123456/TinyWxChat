#include "ChatWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QPixmap>
#include <QBuffer>
#include <QByteArray>
#include "EmojiPicker.h"
#include <QDebug>

ChatWidget::ChatWidget(WeChatSocket* socket, int myUserId,
                       const QString& myNickname, QWidget* parent)
    : QWidget(parent)
    , m_socket(socket)
    , m_myUserId(myUserId)
    , m_myNickname(myNickname)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── 顶部：标题 ──
    auto* topBar = new QHBoxLayout();
    topBar->setContentsMargins(24, 14, 24, 14);
    m_titleLabel = new QLabel();
    m_titleLabel->setStyleSheet("font-size: 20px; font-weight: 900; color: #202a31; background: transparent;");
    topBar->addWidget(m_titleLabel, 1);
    mainLayout->addLayout(topBar);

    auto* sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #dde3e8; background: #dde3e8;");
    mainLayout->addWidget(sep);

    // ── 消息列表 ──
    m_messageList = new QListWidget();
    m_messageList->setStyleSheet(
        "QListWidget { border: none; background: #fbfcfd; color: #202a31; outline: none; }"
        "QListWidget::item { border: none; padding: 4px 18px; }"
    );
    m_messageList->setSpacing(6);
    mainLayout->addWidget(m_messageList, 1);

    // ── 底部输入 ──
    auto* composer = new QWidget();
    composer->setStyleSheet("background: #ffffff; border-top: 1px solid #dde3e8;");
    auto* composerLayout = new QVBoxLayout(composer);
    composerLayout->setContentsMargins(0, 0, 0, 0);
    composerLayout->setSpacing(0);

    auto* toolRow = new QHBoxLayout();
    toolRow->setContentsMargins(20, 12, 20, 0);
    toolRow->setSpacing(8);

    auto* emojiBtn = new QPushButton("😊");
    emojiBtn->setFixedSize(40, 40);
    emojiBtn->setCursor(Qt::PointingHandCursor);
    emojiBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dde3e8; border-radius: 8px;"
        "  background: #ffffff; color: #202a31; font-size: 18px; }"
        "QPushButton:hover { background: #f5f6f8; }"
    );
    toolRow->addWidget(emojiBtn);

    auto* imageBtn = new QPushButton("📎");
    imageBtn->setFixedSize(40, 40);
    imageBtn->setCursor(Qt::PointingHandCursor);
    imageBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dde3e8; border-radius: 8px;"
        "  background: #ffffff; color: #202a31; font-size: 17px; }"
        "QPushButton:hover { background: #f5f6f8; }"
    );
    toolRow->addWidget(imageBtn);
    toolRow->addStretch();
    composerLayout->addLayout(toolRow);

    auto* bottomBar = new QHBoxLayout();
    bottomBar->setContentsMargins(20, 12, 20, 18);
    bottomBar->setSpacing(12);

    m_inputEdit = new QLineEdit();
    m_inputEdit->setPlaceholderText("输入消息，按 Enter 发送");
    m_inputEdit->setMinimumHeight(46);
    m_inputEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dde3e8; border-radius: 8px;"
        "  padding: 0 16px; font-size: 16px; color: #202a31; background: #ffffff;"
        "  selection-background-color: #27c277; }"
        "QLineEdit:focus { border-color: rgba(39,194,119,0.6); }"
    );
    m_sendBtn = new QPushButton("发送 ▸");
    m_sendBtn->setFixedSize(90, 46);
    m_sendBtn->setCursor(Qt::PointingHandCursor);
    m_sendBtn->setStyleSheet(
        "QPushButton { border: none; border-radius: 8px;"
        "  color: #ffffff; background: #27c277; font-weight: 900; font-size: 16px; }"
        "QPushButton:hover { background: #158a57; }"
        "QPushButton:disabled { color: #ffffff; background: #b7c9bf; }"
    );
    bottomBar->addWidget(m_inputEdit, 1);
    bottomBar->addWidget(m_sendBtn);
    composerLayout->addLayout(bottomBar);
    mainLayout->addWidget(composer);

    // 信号
    connect(m_sendBtn, &QPushButton::clicked, this, &ChatWidget::onSendClicked);
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &ChatWidget::onSendClicked);
    connect(m_socket, &WeChatSocket::frameReceived, this, &ChatWidget::onFrameReceived);
    connect(m_socket, &WeChatSocket::connectionFailed, this, &ChatWidget::onConnectionFailed);

    // 表情选择器
    connect(emojiBtn, &QPushButton::clicked, this, [this, emojiBtn]() {
        auto* picker = new EmojiPicker(emojiBtn);
        picker->move(emojiBtn->mapToGlobal(QPoint(0, -picker->height())));
        connect(picker, &EmojiPicker::emojiSelected, this, [this](const QString& emoji) {
            m_inputEdit->insert(emoji);
        });
        picker->show();
    });

    // 图片选择（base64 方案：§0.1 item 4）
    connect(imageBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "选择图片",
            QString(), "Images (*.png *.jpg *.jpeg *.gif *.bmp)");
        if (path.isEmpty()) return;

        // 读取文件
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "图片发送失败", "无法读取文件：" + path);
            return;
        }
        QByteArray fileData = file.readAll();
        file.close();

        // 500KB 限制（§4 Phase 3）
        if (fileData.size() > 500 * 1024) {
            QMessageBox::warning(this, "图片过大",
                QString("图片大小为 %1KB，超过 500KB 限制。\n请压缩或缩小图片后重试。")
                    .arg(fileData.size() / 1024));
            return;
        }

        // Base64 编码
        QByteArray base64 = fileData.toBase64();
        QString base64Str = QString::fromLatin1(base64);

        // 发送（content = base64, msg_type=3）
        QJsonObject msg = MessageBuilder::buildChatSend(m_targetUserId, base64Str, 3);
        msg["seq"] = static_cast<int>(m_socket->nextSeq());
        if (m_socket->sendFrame(FrameType::REQUEST, msg)) {
            appendImage(true, m_myNickname, base64Str);
        }
    });
}

void ChatWidget::openConversation(int userId, const QString& nickname)
{
    m_targetUserId   = userId;
    m_targetNickname = nickname;
    m_messageList->clear();
    m_titleLabel->setText(nickname);
    m_inputEdit->setEnabled(true);
    m_sendBtn->setEnabled(true);
    appendSystem(QString("与 %1 的聊天").arg(nickname));
    pullHistory();
}

void ChatWidget::pullHistory()
{
    if (m_targetUserId <= 0) return;
    QJsonObject msg = MessageBuilder::buildChatHistory(m_targetUserId);
    msg["seq"] = static_cast<int>(m_socket->nextSeq());
    m_socket->sendFrame(FrameType::REQUEST, msg);
}

void ChatWidget::onSendClicked()
{
    QString content = m_inputEdit->text().trimmed();
    if (content.isEmpty() || m_targetUserId <= 0) return;

    // 消息长度 4096 字节校验（§8 UI 规范）
    if (content.toUtf8().size() > 4096) {
        QMessageBox::warning(this, "消息过长",
            QString("消息内容超过 4096 字节限制（当前 %1 字节）。请缩短消息。")
                .arg(content.toUtf8().size()));
        return;
    }

    QJsonObject msg = MessageBuilder::buildChatSend(m_targetUserId, content);
    msg["seq"] = static_cast<int>(m_socket->nextSeq());
    if (m_socket->sendFrame(FrameType::REQUEST, msg)) {
        appendMessage(true, m_myNickname, content);
        m_inputEdit->clear();
    } else {
        appendSystem("发送失败：连接已断开");
    }
}

void ChatWidget::onFrameReceived(const Frame& frame)
{
    QString type = frame.payload.value("type").toString();

    if (type == MsgType::RECV) {
        ChatRecv r = MessageBuilder::parseChatRecv(frame.payload);
        if (r.fromUserId == m_targetUserId || m_targetUserId == 0) {
            if (r.msgType == 3) {
                appendImage(false, r.fromNickname, r.content);
            } else {
                appendMessage(false, r.fromNickname, r.content, r.extra);
            }
        }

    } else if (type == MsgType::SEND_RES) {
        ChatSendResponse r = MessageBuilder::parseChatSendResponse(frame.payload);
        if (!r.ok) {
            appendSystem(QString("发送失败：%1 (code=%2)").arg(r.errorMsg).arg(r.code));
        }

    } else if (type == MsgType::HISTORY_RES) {
        ChatHistoryResponse r = MessageBuilder::parseChatHistoryResponse(frame.payload);
        if (!r.ok) {
            appendSystem(QString("历史拉取失败：%1").arg(r.errorMsg));
            return;
        }
        if (r.messages.empty()) {
            appendSystem("（无历史消息）");
        } else {
            appendSystem(QString("—— 历史消息（共 %1 条）——").arg(r.messages.size()));
            for (const auto& m : r.messages) {
                bool isMine = (m.fromUserId == m_myUserId);
                QString sender = isMine ? m_myNickname : m_targetNickname;
                if (m.msgType == 3) {
                    appendImage(isMine, sender, m.content);
                } else {
                    appendMessage(isMine, sender, m.content, m.extra);
                }
            }
            appendSystem("—— 以上为历史消息 ——");
        }

    } else if (type == MsgType::ERROR) {
        ErrorNotification e = MessageBuilder::parseError(frame.payload);
        appendSystem(QString("服务器错误：%1 (code=%2)").arg(e.message).arg(e.code));
    }
}

void ChatWidget::onConnectionFailed(const QString& reason)
{
    appendSystem(QString("连接断开：%1").arg(reason));
    m_sendBtn->setEnabled(false);
}

// ── 消息展示 ─────────────────────────────────────────

void ChatWidget::appendMessage(bool isMine, const QString& sender, const QString& content,
                                const QString& extra)
{
    QString prefix = isMine ? "我" : sender;
    QString color  = isMine ? "#2e6fa7" : "#158a57";
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm");
    QString displayText = content;
    if (!extra.isEmpty() && content.startsWith("[表情]")) {
        displayText = extra;
    }

    QString display = QString("<b style='color:%1; font-size: 14px;'>%2</b> "
                              "<span style='color:#7d8c96; font-size: 12px;'>%3</span><br>"
                              "<span style='color:#202a31; font-size: 15px;'>%4</span>")
                          .arg(color, prefix.toHtmlEscaped(), timeStr,
                               displayText.toHtmlEscaped());

    auto* item = new QListWidgetItem();
    auto* row = new QWidget();
    row->setStyleSheet("background: transparent;");
    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(8, 6, 8, 6);
    rowLayout->setSpacing(0);

    auto* label = new QLabel(display);
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    label->setMaximumWidth(520);
    label->setContentsMargins(12, 10, 12, 10);
    label->setStyleSheet(QString(
        "QLabel { color: #202a31; background: %1; border: 1px solid %2;"
        "  border-radius: 12px; padding: 10px 14px; }"
    ).arg(isMine ? "#d6eaf8" : "#d5f5e3",
          isMine ? "#bdddf3" : "#bee9cf"));

    if (isMine) {
        rowLayout->addStretch();
        rowLayout->addWidget(label);
    } else {
        rowLayout->addWidget(label);
        rowLayout->addStretch();
    }

    m_messageList->addItem(item);
    item->setSizeHint(QSize(0, qMax(70, label->sizeHint().height() + 22)));
    m_messageList->setItemWidget(item, row);
    m_messageList->scrollToBottom();
}

void ChatWidget::appendImage(bool isMine, const QString& sender, const QString& base64Data)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm");
    QColor bgColor = isMine ? QColor("#d6eaf8") : QColor("#d5f5e3");

    auto* item = new QListWidgetItem();
    auto* row = new QWidget();
    row->setStyleSheet("background: transparent;");
    auto* hbox = new QHBoxLayout(row);
    hbox->setContentsMargins(8, 6, 8, 6);
    hbox->setSpacing(0);

    auto* card = new QWidget();
    card->setStyleSheet(QString(
        "QWidget { background: %1; border: 1px solid %2; border-radius: 12px; }"
        "QLabel { background: transparent; border: none; color: #202a31; }"
    ).arg(bgColor.name(), isMine ? "#bdddf3" : "#bee9cf"));
    auto* vbox = new QVBoxLayout(card);
    vbox->setContentsMargins(12, 10, 12, 12);
    vbox->setSpacing(8);

    // 解码 base64 → QImage → QPixmap
    QByteArray raw = QByteArray::fromBase64(base64Data.toLatin1());
    QImage img;
    img.loadFromData(raw);

    QLabel* imgLabel = new QLabel();
    if (!img.isNull()) {
        QPixmap pix = QPixmap::fromImage(img);
        // 限制最大显示尺寸 200x200
        if (pix.width() > 200 || pix.height() > 200) {
            pix = pix.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        imgLabel->setPixmap(pix);
        item->setSizeHint(QSize(0, pix.height() + 24));
    } else {
        imgLabel->setText("[图片加载失败]");
        imgLabel->setStyleSheet("color: #7d8c96; font-size: 13px; font-weight: 700;");
        item->setSizeHint(QSize(0, 40));
    }

    QString senderColor = isMine ? "#2e6fa7" : "#158a57";
    QLabel* nameLabel = new QLabel(QString("<b style='color:%1; font-size:14px;'>%2</b> "
                                           "<span style='color:#7d8c96; font-size:12px;'>%3</span>")
                                       .arg(senderColor, sender.toHtmlEscaped(), timeStr));
    nameLabel->setTextFormat(Qt::RichText);

    vbox->addWidget(nameLabel);
    vbox->addWidget(imgLabel);

    if (isMine) {
        hbox->addStretch();
        hbox->addWidget(card);
    } else {
        hbox->addWidget(card);
        hbox->addStretch();
    }

    m_messageList->addItem(item);
    item->setSizeHint(QSize(0, qMax(item->sizeHint().height(), card->sizeHint().height() + 20)));
    m_messageList->setItemWidget(item, row);
    m_messageList->scrollToBottom();
}

void ChatWidget::appendSystem(const QString& text)
{
    auto* item = new QListWidgetItem();
    item->setSizeHint(QSize(0, 34));
    auto* label = new QLabel(QString("<span style='color:#71808a; font-size: 13px; font-weight: 700;'>%1</span>")
                                 .arg(text.toHtmlEscaped()));
    label->setAlignment(Qt::AlignCenter);
    label->setTextFormat(Qt::RichText);
    label->setStyleSheet("background: transparent;");
    m_messageList->addItem(item);
    m_messageList->setItemWidget(item, label);
    m_messageList->scrollToBottom();
}
