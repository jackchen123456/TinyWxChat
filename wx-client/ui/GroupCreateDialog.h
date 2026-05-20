#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include "../network/WeChatSocket.h"

// 创建群聊对话框
class GroupCreateDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GroupCreateDialog(WeChatSocket* socket, QWidget* parent = nullptr);

    int groupId() const { return m_groupId; }
    QString groupName() const { return m_name; }

signals:
    void groupCreated(int groupId, const QString& name);

private slots:
    void onCreateClicked();

private:
    WeChatSocket* m_socket;
    QLineEdit*    m_nameEdit;
    QLabel*       m_statusLabel;
    int           m_groupId = 0;
    QString       m_name;
};
