#include "ChatWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QDebug>

ChatWidget::ChatWidget(WeChatSocket* socket, int myUserId,
                       const QString& myNickname, QWidget* parent)
    : QWidget(parent)
    , m_socket(socket)
    , m_myUserId(myUserId)
    , m_myNickname(myNickname)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // ── 顶部：目标用户 ID ──
    auto* topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("发送给用户ID:"));
    m_targetEdit = new QLineEdit();
    m_targetEdit->setPlaceholderText("输入目标用户ID（如 2）");
    m_targetEdit->setMaximumWidth(80);
    topLayout->addWidget(m_targetEdit);
    topLayout->addStretch();

    m_statusLabel = new QLabel("已连接");
    m_statusLabel->setStyleSheet("color: #27ae60;");
    topLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(topLayout);

    // ── 消息列表 ──
    m_messageList = new QListWidget();
    m_messageList->setStyleSheet(
        "QListWidget { border: 1px solid #ddd; border-radius: 4px; background: #f5f5f5; }"
    );
    mainLayout->addWidget(m_messageList, 1); // stretch=1

    // ── 底部：输入框 + 发送按钮 ──
    auto* bottomLayout = new QHBoxLayout();
    m_inputEdit = new QLineEdit();
    m_inputEdit->setPlaceholderText("输入消息...");
    m_inputEdit->setMinimumHeight(32);
    bottomLayout->addWidget(m_inputEdit, 1);

    m_sendBtn = new QPushButton("发送");
    m_sendBtn->setMinimumHeight(32);
    m_sendBtn->setMinimumWidth(70);
    bottomLayout->addWidget(m_sendBtn);
    mainLayout->addLayout(bottomLayout);

    // 信号连接
    connect(m_sendBtn, &QPushButton::clicked, this, &ChatWidget::onSendClicked);
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &ChatWidget::onSendClicked);
    connect(m_socket, &WeChatSocket::frameReceived, this, &ChatWidget::onFrameReceived);
    connect(m_socket, &WeChatSocket::connectionFailed, this, &ChatWidget::onConnectionFailed);

    appendSystem(QString("已登录：%1 (ID: %2)").arg(m_myNickname).arg(m_myUserId));
    appendSystem("请输入目标用户ID后开始聊天");
}

void ChatWidget::pullHistory(int withUserId)
{
    QJsonObject msg = MessageBuilder::buildChatHistory(withUserId);
    msg["seq"] = static_cast<int>(m_socket->nextSeq());
    m_socket->sendFrame(FrameType::REQUEST, msg);
    appendSystem(QString("正在拉取与用户 %1 的历史消息...").arg(withUserId));
}

void ChatWidget::onSendClicked()
{
    QString content = m_inputEdit->text().trimmed();
    QString targetText = m_targetEdit->text().trimmed();

    if (content.isEmpty()) return;
    if (targetText.isEmpty()) {
        appendSystem("请先输入目标用户ID！");
        return;
    }

    bool ok = false;
    int targetId = targetText.toInt(&ok);
    if (!ok || targetId <= 0) {
        appendSystem("用户ID 必须为正整数");
        return;
    }

    // 构造并发送
    QJsonObject msg = MessageBuilder::buildChatSend(targetId, content);
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

    if (type == "chat.recv") {
        ChatRecv r = MessageBuilder::parseChatRecv(frame.payload);
        appendMessage(false, r.fromNickname, r.content);

    } else if (type == "chat.send_res") {
        ChatSendResponse r = MessageBuilder::parseChatSendResponse(frame.payload);
        if (!r.ok) {
            appendSystem(QString("发送失败：%1 (code=%2)").arg(r.errorMsg).arg(r.code));
        }

    } else if (type == "chat.history_res") {
        ChatHistoryResponse r = MessageBuilder::parseChatHistoryResponse(frame.payload);
        if (!r.ok) {
            appendSystem(QString("历史拉取失败：%1 (code=%2)").arg(r.errorMsg).arg(r.code));
            return;
        }
        if (r.messages.empty()) {
            appendSystem("（无历史消息）");
        } else {
            appendSystem(QString("—— 历史消息（共 %1 条）——").arg(r.messages.size()));
            // 历史消息按时间排序（正序展示）
            for (const auto& m : r.messages) {
                bool isMine = (m.fromUserId == m_myUserId);
                QString sender = isMine ? m_myNickname : QString("用户%1").arg(m.fromUserId);
                appendMessage(isMine, sender, m.content);
            }
            appendSystem("—— 以上为历史消息 ——");
        }

    } else if (type == "error") {
        ErrorNotification e = MessageBuilder::parseError(frame.payload);
        appendSystem(QString("服务器错误：%1 (code=%2)").arg(e.message).arg(e.code));
    }
}

void ChatWidget::onConnectionFailed(const QString& reason)
{
    appendSystem(QString("连接断开：%1").arg(reason));
    m_statusLabel->setText("已断开");
    m_statusLabel->setStyleSheet("color: #e74c3c;");
    m_sendBtn->setEnabled(false);
}

// ── 辅助方法 ─────────────────────────────────────────

void ChatWidget::appendMessage(bool isMine, const QString& sender, const QString& content)
{
    QString prefix = isMine ? "我" : sender;
    QString color  = isMine ? "#2e86c1" : "#27ae60";
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm");

    QString display = QString("<b style='color:%1'>%2</b> "
                              "<span style='color:#888; font-size:11px;'>%3</span><br>"
                              "%4")
                          .arg(color, prefix, timeStr, content.toHtmlEscaped());

    auto* item = new QListWidgetItem();
    item->setSizeHint(QSize(0, 48));

    auto* label = new QLabel(display);
    label->setWordWrap(true);
    label->setStyleSheet(QString(
        "QLabel { background: %1; border-radius: 6px; padding: 6px 10px; }"
    ).arg(isMine ? "#d6eaf8" : "#d5f5e3"));

    m_messageList->addItem(item);
    m_messageList->setItemWidget(item, label);
    m_messageList->scrollToBottom();
}

void ChatWidget::appendSystem(const QString& text)
{
    auto* item = new QListWidgetItem();
    item->setSizeHint(QSize(0, 24));

    auto* label = new QLabel(QString("<span style='color:#999; font-size:12px;'>%1</span>")
                                 .arg(text.toHtmlEscaped()));
    label->setAlignment(Qt::AlignCenter);

    m_messageList->addItem(item);
    m_messageList->setItemWidget(item, label);
    m_messageList->scrollToBottom();
}
