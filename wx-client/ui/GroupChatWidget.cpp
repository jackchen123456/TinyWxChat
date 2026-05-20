#include "GroupChatWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QPixmap>
#include <QBuffer>
#include "EmojiPicker.h"
#include <QDebug>

GroupChatWidget::GroupChatWidget(WeChatSocket* socket, int myUserId,
                                 const QString& myNickname, QWidget* parent)
    : QWidget(parent)
    , m_socket(socket)
    , m_myUserId(myUserId)
    , m_myNickname(myNickname)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* topBar = new QHBoxLayout();
    topBar->setContentsMargins(8, 4, 8, 4);
    m_backBtn = new QPushButton("← 返回");
    m_backBtn->setFlat(true);
    m_backBtn->setStyleSheet("font-size: 14px; color: #333; border: none;");
    m_titleLabel = new QLabel("群聊");
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    topBar->addWidget(m_backBtn);
    topBar->addWidget(m_titleLabel, 1);
    mainLayout->addLayout(topBar);

    auto* sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #ddd;");
    mainLayout->addWidget(sep);

    m_messageList = new QListWidget();
    m_messageList->setStyleSheet("QListWidget { border: none; background: #f0f0f0; }");
    mainLayout->addWidget(m_messageList, 1);

    auto* bottomBar = new QHBoxLayout();
    bottomBar->setContentsMargins(8, 6, 8, 6);

    auto* emojiBtn = new QPushButton("😊");
    emojiBtn->setFixedSize(34, 34);
    emojiBtn->setStyleSheet("font-size: 18px; border: none;");
    bottomBar->addWidget(emojiBtn);

    auto* imageBtn = new QPushButton("📎");
    imageBtn->setFixedSize(34, 34);
    imageBtn->setStyleSheet("font-size: 16px; border: none;");
    bottomBar->addWidget(imageBtn);

    m_inputEdit = new QLineEdit();
    m_inputEdit->setPlaceholderText("输入群消息...");
    m_inputEdit->setMinimumHeight(34);
    m_sendBtn = new QPushButton("发送");
    m_sendBtn->setMinimumHeight(34);
    m_sendBtn->setMinimumWidth(60);
    bottomBar->addWidget(m_inputEdit, 1);
    bottomBar->addWidget(m_sendBtn);
    mainLayout->addLayout(bottomBar);

    connect(m_sendBtn, &QPushButton::clicked, this, &GroupChatWidget::onSendClicked);
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &GroupChatWidget::onSendClicked);
    connect(m_backBtn, &QPushButton::clicked, this, &GroupChatWidget::back);
    connect(m_socket, &WeChatSocket::frameReceived, this, &GroupChatWidget::onFrameReceived);
    connect(m_socket, &WeChatSocket::connectionFailed, this, &GroupChatWidget::onConnectionFailed);

    connect(emojiBtn, &QPushButton::clicked, this, [this, emojiBtn]() {
        auto* picker = new EmojiPicker(emojiBtn);
        picker->move(emojiBtn->mapToGlobal(QPoint(0, -picker->height())));
        connect(picker, &EmojiPicker::emojiSelected, this, [this](const QString& emoji) {
            m_inputEdit->insert(emoji);
        });
        picker->show();
    });

    connect(imageBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "选择图片",
            QString(), "Images (*.png *.jpg *.jpeg *.gif *.bmp)");
        if (path.isEmpty()) return;

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "图片发送失败", "无法读取文件：" + path);
            return;
        }
        QByteArray fileData = file.readAll();
        file.close();
        if (fileData.size() > 500 * 1024) {
            QMessageBox::warning(this, "图片过大",
                QString("图片大小为 %1KB，超过 500KB 限制。").arg(fileData.size() / 1024));
            return;
        }
        QByteArray base64 = fileData.toBase64();
        QJsonObject msg = MessageBuilder::buildGroupSend(m_groupId, QString::fromLatin1(base64), 3);
        msg["seq"] = static_cast<int>(m_socket->nextSeq());
        if (m_socket->sendFrame(FrameType::REQUEST, msg)) {
            appendImage(m_myNickname, QString::fromLatin1(base64));
        }
    });
}

void GroupChatWidget::openGroup(int groupId, const QString& name)
{
    m_groupId   = groupId;
    m_groupName = name;
    m_messageList->clear();
    m_titleLabel->setText(QString("%1 (群ID:%2)").arg(name).arg(groupId));
    m_inputEdit->setEnabled(true);
    m_sendBtn->setEnabled(true);
    appendSystem(QString("进入群聊「%1」").arg(name));

    QJsonObject msg = MessageBuilder::buildGroupHistory(groupId);
    msg["seq"] = static_cast<int>(m_socket->nextSeq());
    m_socket->sendFrame(FrameType::REQUEST, msg);
}

void GroupChatWidget::onSendClicked()
{
    QString content = m_inputEdit->text().trimmed();
    if (content.isEmpty() || m_groupId <= 0) return;

    if (content.toUtf8().size() > 4096) {
        QMessageBox::warning(this, "消息过长",
            QString("消息内容超过 4096 字节限制（当前 %1 字节）。").arg(content.toUtf8().size()));
        return;
    }

    QJsonObject msg = MessageBuilder::buildGroupSend(m_groupId, content);
    msg["seq"] = static_cast<int>(m_socket->nextSeq());
    if (m_socket->sendFrame(FrameType::REQUEST, msg)) {
        appendMessage(m_myNickname, content);
        m_inputEdit->clear();
    } else {
        appendSystem("发送失败：连接已断开");
    }
}

void GroupChatWidget::onFrameReceived(const Frame& frame)
{
    QString type = frame.payload.value("type").toString();

    if (type == "group.recv") {
        GroupRecv r = MessageBuilder::parseGroupRecv(frame.payload);
        if (r.groupId == m_groupId) {
            if (r.msgType == 3) {
                appendImage(r.fromNickname, r.content);
            } else {
                appendMessage(r.fromNickname, r.content, r.extra);
            }
        }

    } else if (type == "group.send_res") {
        GroupSendResponse r = MessageBuilder::parseGroupSendResponse(frame.payload);
        if (!r.ok) {
            appendSystem(QString("发送失败：%1").arg(r.errorMsg));
        }

    } else if (type == "group.history_res") {
        GroupHistoryResponse r = MessageBuilder::parseGroupHistoryResponse(frame.payload);
        if (!r.ok) {
            appendSystem(QString("历史拉取失败：%1").arg(r.errorMsg));
            return;
        }
        if (r.messages.empty()) {
            appendSystem("（暂无群消息）");
        } else {
            appendSystem(QString("—— 群聊历史（共 %1 条）——").arg(r.messages.size()));
            for (const auto& m : r.messages) {
                if (m.msgType == 3) {
                    appendImage(m.fromNickname, m.content);
                } else {
                    appendMessage(m.fromNickname, m.content, m.extra);
                }
            }
            appendSystem("—— 以上为历史消息 ——");
        }

    } else if (type == "error") {
        ErrorNotification e = MessageBuilder::parseError(frame.payload);
        appendSystem(QString("错误：%1").arg(e.message));
    }
}

void GroupChatWidget::onConnectionFailed(const QString& reason)
{
    appendSystem(QString("连接断开：%1").arg(reason));
    m_sendBtn->setEnabled(false);
}

void GroupChatWidget::appendMessage(const QString& sender, const QString& content,
                                     const QString& extra)
{
    bool isMine = (sender == m_myNickname);
    QString color = isMine ? "#2e86c1" : "#8e44ad";
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm");
    QString display = QString("<b style='color:%1'>%2</b> "
                              "<span style='color:#888; font-size:11px;'>%3</span><br>%4")
                          .arg(color, sender.toHtmlEscaped(), timeStr,
                               content.toHtmlEscaped());

    auto* item = new QListWidgetItem();
    item->setSizeHint(QSize(0, 48));
    auto* label = new QLabel(display);
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    label->setStyleSheet(QString(
        "QLabel { background: %1; border-radius: 6px; padding: 6px 10px; }"
    ).arg(isMine ? "#d6eaf8" : "#ebdef0"));

    m_messageList->addItem(item);
    m_messageList->setItemWidget(item, label);
    m_messageList->scrollToBottom();
}

void GroupChatWidget::appendImage(const QString& sender, const QString& base64Data)
{
    bool isMine = (sender == m_myNickname);
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm");
    QColor bgColor = isMine ? QColor("#d6eaf8") : QColor("#fdebd0");

    auto* item = new QListWidgetItem();
    auto* widget = new QWidget();
    auto* hbox = new QHBoxLayout(widget);
    hbox->setContentsMargins(6, 4, 6, 4);

    QByteArray raw = QByteArray::fromBase64(base64Data.toLatin1());
    QImage img;
    img.loadFromData(raw);

    QLabel* imgLabel = new QLabel();
    if (!img.isNull()) {
        QPixmap pix = QPixmap::fromImage(img);
        if (pix.width() > 200 || pix.height() > 200)
            pix = pix.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        imgLabel->setPixmap(pix);
        item->setSizeHint(QSize(0, pix.height() + 24));
    } else {
        imgLabel->setText("[图片加载失败]");
        imgLabel->setStyleSheet("color: #999; font-size: 12px;");
        item->setSizeHint(QSize(0, 40));
    }

    QString senderColor = isMine ? "#2e86c1" : "#8e44ad";
    QLabel* nameLabel = new QLabel(QString("<b style='color:%1'>%2</b> "
                                           "<span style='color:#888; font-size:11px;'>%3</span>")
                                       .arg(senderColor, sender.toHtmlEscaped(), timeStr));
    nameLabel->setTextFormat(Qt::RichText);

    auto* vbox = new QVBoxLayout();
    vbox->setSpacing(2);
    vbox->addWidget(nameLabel);
    vbox->addWidget(imgLabel);

    if (isMine) {
        hbox->addStretch();
        hbox->addLayout(vbox);
    } else {
        hbox->addLayout(vbox);
        hbox->addStretch();
    }

    widget->setStyleSheet(QString("background: %1; border-radius: 6px;").arg(bgColor.name()));

    m_messageList->addItem(item);
    m_messageList->setItemWidget(item, widget);
    m_messageList->scrollToBottom();
}

void GroupChatWidget::appendSystem(const QString& text)
{
    auto* item = new QListWidgetItem();
    item->setSizeHint(QSize(0, 24));
    auto* label = new QLabel(QString("<span style='color:#999; font-size:12px;'>%1</span>")
                                 .arg(text.toHtmlEscaped()));
    label->setAlignment(Qt::AlignCenter);
    label->setTextFormat(Qt::RichText);
    m_messageList->addItem(item);
    m_messageList->setItemWidget(item, label);
    m_messageList->scrollToBottom();
}
