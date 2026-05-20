#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include "../network/WeChatSocket.h"

class GroupListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GroupListWidget(WeChatSocket* socket, int myUserId, QWidget* parent = nullptr);

signals:
    void openGroupChat(int groupId, const QString& name);

private slots:
    void onCreateGroup();
    void onApplyGroup();  // 申请加入群聊
    void onFrameReceived(const Frame& frame);

private:
    WeChatSocket* m_socket;
    int           m_myUserId;
    QListWidget*  m_listWidget;
    QPushButton*  m_createBtn;
    QLineEdit*    m_applyEdit;
    QPushButton*  m_applyBtn;
    QLabel*       m_statusLabel;

    // 群聊申请待处理列表
    QListWidget*  m_pendingApplyList;
    QLabel*       m_pendingApplyLabel;
};
