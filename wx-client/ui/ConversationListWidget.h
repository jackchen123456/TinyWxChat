#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QMap>
#include "../network/WeChatSocket.h"

struct ConversationItem {
    int     userId = 0;
    QString nickname;
    QString lastMsg;
    int64_t timestamp = 0;
};

class ConversationListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConversationListWidget(WeChatSocket* socket, QWidget* parent = nullptr);

    void onMessageReceived(int fromUserId, const QString& fromNickname,
                           const QString& content, int64_t timestamp);
    void onMessageSent(int toUserId, const QString& toNickname,
                       const QString& content, int64_t timestamp);
    void setConversations(const QList<ConversationItem>& items);

signals:
    void conversationClicked(int userId, const QString& nickname);

private:
    void refreshList();
    void insertOrUpdate(int userId, const QString& nickname,
                        const QString& lastMsg, int64_t timestamp);

    WeChatSocket* m_socket;
    QListWidget*  m_listWidget;
    QLabel*       m_emptyLabel;
    QMap<int, ConversationItem> m_conversations;
};
