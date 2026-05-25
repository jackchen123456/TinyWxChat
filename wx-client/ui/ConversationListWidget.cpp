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
        "QListWidget { border: none; background: transparent; color: #202a31; outline: none; }"
        "QListWidget::item { border-bottom: 1px solid #edf1f4; padding: 6px 4px; color: #202a31; }"
        "QListWidget::item:hover { background: #f5f6f8; border-radius: 6px; }"
        "QListWidget::item:selected { background: #e7f8ef; border-radius: 6px; color: #202a31; }"
    );
    layout->addWidget(m_listWidget);

    m_emptyLabel = new QLabel("暂无会话\n\n点击好友发起聊天");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("color: #6f7f8b; font-size: 15px; font-weight: 700; background: transparent;");
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
        item->setSizeHint(QSize(0, 60));
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
        auto* nameLabel = new QLabel(c.nickname);
        nameLabel->setStyleSheet("font-size: 15px; font-weight: 900; color: #202a31; background: transparent;");
        auto* msgLabel = new QLabel(c.lastMsg.left(30).toHtmlEscaped());
        msgLabel->setStyleSheet("color: #6f7f8b; font-size: 13px; font-weight: 700; background: transparent;");
        msgLabel->setWordWrap(true);
        vbox->addWidget(nameLabel);
        vbox->addWidget(msgLabel);
        hbox->addLayout(vbox, 1);

        // 时间
        QString timeStr = QDateTime::fromSecsSinceEpoch(c.timestamp).toString("MM-dd HH:mm");
        auto* timeLabel = new QLabel(timeStr);
        timeLabel->setStyleSheet("color: #7d8c96; font-size: 12px; font-weight: 700; background: transparent;");
        hbox->addWidget(timeLabel);

        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, widget);
    }
}
