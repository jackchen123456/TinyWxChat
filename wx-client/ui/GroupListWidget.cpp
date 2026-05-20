#include "GroupListWidget.h"
#include "GroupCreateDialog.h"
#include "AvatarLabel.h"
#include "../protocol/MessageBuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QDebug>

GroupListWidget::GroupListWidget(WeChatSocket* socket, int myUserId, QWidget* parent)
    : QWidget(parent)
    , m_socket(socket)
    , m_myUserId(myUserId)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    // 创建群聊
    m_createBtn = new QPushButton("+ 创建群聊");
    m_createBtn->setStyleSheet("background: #07c160; color: white; font-size: 14px; padding: 6px;");
    layout->addWidget(m_createBtn);

    // 申请加入群聊
    auto* applyLayout = new QHBoxLayout();
    applyLayout->addWidget(new QLabel("申请加入群聊："));
    m_applyEdit = new QLineEdit();
    m_applyEdit->setPlaceholderText("输入群ID");
    m_applyBtn = new QPushButton("申请加入");
    applyLayout->addWidget(m_applyEdit);
    applyLayout->addWidget(m_applyBtn);
    layout->addLayout(applyLayout);

    // 群聊列表
    m_listWidget = new QListWidget();
    m_listWidget->setStyleSheet(
        "QListWidget { border: 1px solid #ddd; border-radius: 4px; }"
        "QListWidget::item { padding: 6px; border-bottom: 1px solid #eee; }"
        "QListWidget::item:hover { background: #f0f0f0; }"
    );
    m_listWidget->addItem("（暂无群聊，点击上方按钮创建或申请加入）");
    layout->addWidget(m_listWidget, 1);

    // 群聊申请待处理通知（§2 群聊申请 查看/同意）
    m_pendingApplyLabel = new QLabel("待处理的群聊申请 (0)");
    m_pendingApplyLabel->setStyleSheet("font-weight: bold; margin-top: 8px;");
    layout->addWidget(m_pendingApplyLabel);

    m_pendingApplyList = new QListWidget();
    m_pendingApplyList->setStyleSheet(
        "QListWidget { border: 1px solid #f0c040; border-radius: 4px; background: #fffbeb; }"
        "QListWidget::item { padding: 6px; }"
    );
    layout->addWidget(m_pendingApplyList, 0);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: #888; font-size: 12px;");
    layout->addWidget(m_statusLabel);

    connect(m_createBtn, &QPushButton::clicked, this, &GroupListWidget::onCreateGroup);
    connect(m_applyBtn, &QPushButton::clicked, this, &GroupListWidget::onApplyGroup);
    connect(m_socket, &WeChatSocket::frameReceived, this, &GroupListWidget::onFrameReceived);

    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        int groupId = item->data(Qt::UserRole).toInt();
        QString name = item->data(Qt::UserRole + 1).toString();
        if (groupId > 0) emit openGroupChat(groupId, name);
    });

    // 双击待处理申请 → 自动同意
    connect(m_pendingApplyList, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem* item) {
        int requestId = item->data(Qt::UserRole).toInt();
        QJsonObject msg = MessageBuilder::buildGroupApplyHandle(requestId, "accept");
        msg["seq"] = static_cast<int>(m_socket->nextSeq());
        m_socket->sendFrame(FrameType::REQUEST, msg);
        m_statusLabel->setText("已同意该群聊申请");
        QTimer::singleShot(1500, [this]() { m_statusLabel->setText(""); });
    });
}

void GroupListWidget::onCreateGroup()
{
    GroupCreateDialog dlg(m_socket, this);
    if (dlg.exec() == QDialog::Accepted) {
        if (m_listWidget->count() == 1 &&
            m_listWidget->item(0)->text().contains("暂无群聊")) {
            m_listWidget->clear();
        }
        auto* item = new QListWidgetItem();
        item->setText(QString("%1 (ID:%2)").arg(dlg.groupName()).arg(dlg.groupId()));
        item->setData(Qt::UserRole, dlg.groupId());
        item->setData(Qt::UserRole + 1, dlg.groupName());
        m_listWidget->insertItem(0, item);
    }
}

void GroupListWidget::onApplyGroup()
{
    QString idText = m_applyEdit->text().trimmed();
    if (idText.isEmpty()) {
        m_statusLabel->setText("请输入群ID");
        return;
    }
    bool ok = false;
    int groupId = idText.toInt(&ok);
    if (!ok || groupId <= 0) {
        m_statusLabel->setText("群ID必须为正整数");
        return;
    }

    QJsonObject msg = MessageBuilder::buildGroupApply(groupId);
    msg["seq"] = static_cast<int>(m_socket->nextSeq());
    m_socket->sendFrame(FrameType::REQUEST, msg);
    m_statusLabel->setText("正在发送加入请求...");

    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(m_socket, &WeChatSocket::frameReceived, this,
                    [this, conn](const Frame& frame) {
        QString type = frame.payload.value("type").toString();
        if (type == "group.apply_res") {
            GroupApplyResponse r = MessageBuilder::parseGroupApplyResponse(frame.payload);
            if (r.ok) {
                m_statusLabel->setStyleSheet("color: #27ae60;");
                m_statusLabel->setText("申请已发送，等待群主审批");
            } else {
                m_statusLabel->setStyleSheet("color: #e74c3c;");
                m_statusLabel->setText(QString("申请失败：%1").arg(r.errorMsg));
            }
            disconnect(*conn);
        }
    });
}

void GroupListWidget::onFrameReceived(const Frame& frame)
{
    QString type = frame.payload.value("type").toString();

    // 群聊申请通知（群主收到）
    if (type == "group.apply_notify") {
        GroupApplyNotify n = MessageBuilder::parseGroupApplyNotify(frame.payload);
        QString label = QString("[%1] %2 申请加入群「%3」（双击同意）")
            .arg(n.requestId).arg(n.fromNickname).arg(n.groupName);
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, n.requestId);
        if (m_pendingApplyList->count() == 0 &&
            m_pendingApplyList->item(0) &&
            m_pendingApplyList->item(0)->text().contains("暂无")) {
            m_pendingApplyList->clear();
        }
        m_pendingApplyList->addItem(item);
        m_pendingApplyLabel->setText(QString("待处理的群聊申请 (%1)").arg(m_pendingApplyList->count()));
    }

    // 群聊申请处理结果
    else if (type == "group.apply_handle_res") {
        GroupApplyHandleResponse r = MessageBuilder::parseGroupApplyHandleResponse(frame.payload);
        if (r.ok) {
            m_statusLabel->setStyleSheet("color: #27ae60;");
            m_statusLabel->setText("已同意群聊申请");
        }
    }
}
