#include "ConversationListWidget.h"
#include "AvatarLabel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QDebug>

ConversationListWidget::ConversationListWidget(WeChatSocket* socket, QWidget* parent)
    : QWidget(parent)
    , m_socket(socket)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_listWidget = new QListWidget();
    m_listWidget->setStyleSheet(
        "QListWidget { border: none; background: white; }"
        "QListWidget::item { border-bottom: 1px solid #eee; padding: 4px; }"
        "QListWidget::item:hover { background: #f0f0f0; }"
    );
    layout->addWidget(m_listWidget);

    m_emptyLabel = new QLabel("暂无会话\n\n发送一条消息或开始聊天即可看到会话");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("color: #999; font-size: 14px;");
    layout->addWidget(m_emptyLabel);
    m_emptyLabel->hide();

    connect(m_listWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        int userId = item->data(Qt::UserRole).toInt();
        QString nickname = item->data(Qt::UserRole + 1).toString();
        emit conversationClicked(userId, nickname);
    });
}

void ConversationListWidget::insertOrUpdate(int userId, const QString& nickname,
                                             const QString& lastMsg, int64_t timestamp)
{
    ConversationItem& c = m_conversations[userId];
    c.userId    = userId;
    c.nickname  = nickname.isEmpty() ? c.nickname : nickname;
    c.lastMsg   = lastMsg;
    c.timestamp = timestamp;
}

void ConversationListWidget::onMessageReceived(int fromUserId, const QString& fromNickname,
                                                const QString& content, int64_t timestamp)
{
    insertOrUpdate(fromUserId, fromNickname, content, timestamp);
    refreshList();
}

void ConversationListWidget::onMessageSent(int toUserId, const QString& toNickname,
                                            const QString& content, int64_t timestamp)
{
    insertOrUpdate(toUserId, toNickname, content, timestamp);
    refreshList();
}

void ConversationListWidget::setConversations(const QList<ConversationItem>& items)
{
    for (const auto& item : items) {
        m_conversations[item.userId] = item;
    }
    refreshList();
}

void ConversationListWidget::refreshList()
{
    m_listWidget->clear();

    if (m_conversations.isEmpty()) {
        m_listWidget->hide();
        m_emptyLabel->show();
        return;
    }

    m_listWidget->show();
    m_emptyLabel->hide();

    QList<ConversationItem> sorted = m_conversations.values();
    std::sort(sorted.begin(), sorted.end(), [](const ConversationItem& a, const ConversationItem& b) {
        return a.timestamp > b.timestamp;
    });

    for (const auto& c : sorted) {
        auto* item = new QListWidgetItem();
        item->setSizeHint(QSize(0, 52));
        item->setData(Qt::UserRole, c.userId);
        item->setData(Qt::UserRole + 1, c.nickname);

        auto* widget = new QWidget();
        auto* hbox = new QHBoxLayout(widget);
        hbox->setContentsMargins(6, 6, 6, 6);
        hbox->setSpacing(10);

        // 头像（§2: 头像展示）
        auto* avatar = new AvatarLabel();
        avatar->setNickname(c.nickname);
        hbox->addWidget(avatar);

        // 中间：昵称 + 最后一条消息
        auto* vbox = new QVBoxLayout();
        vbox->setSpacing(2);
        auto* nameLabel = new QLabel(QString("<b>%1</b>").arg(c.nickname.toHtmlEscaped()));
        nameLabel->setTextFormat(Qt::RichText);
        auto* msgLabel = new QLabel(c.lastMsg.left(30).toHtmlEscaped());
        msgLabel->setStyleSheet("color: #888; font-size: 12px;");
        msgLabel->setTextFormat(Qt::RichText);
        vbox->addWidget(nameLabel);
        vbox->addWidget(msgLabel);
        hbox->addLayout(vbox, 1);

        // 时间
        QString timeStr = QDateTime::fromSecsSinceEpoch(c.timestamp).toString("MM-dd HH:mm");
        auto* timeLabel = new QLabel(timeStr);
        timeLabel->setStyleSheet("color: #aaa; font-size: 11px;");
        hbox->addWidget(timeLabel);

        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, widget);
    }
}
