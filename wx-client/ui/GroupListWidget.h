#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include "../network/WeChatSocket.h"

// 精简版：只负责显示群聊列表，不含标题/按钮等外部 chrome
class GroupListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GroupListWidget(WeChatSocket* socket, int myUserId, QWidget* parent = nullptr);
    void refreshGroups();

signals:
    void openGroupChat(int groupId, const QString& name);

private slots:
    void onFrameReceived(const Frame& frame);

private:
    WeChatSocket* m_socket;
    int           m_myUserId;
    QListWidget*  m_listWidget;
};
