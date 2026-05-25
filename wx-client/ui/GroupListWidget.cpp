#include "GroupListWidget.h"
#include "AvatarLabel.h"
#include "../protocol/MessageBuilder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

GroupListWidget::GroupListWidget(WeChatSocket* socket, int myUserId, QWidget* parent)
    : QWidget(parent)
    , m_socket(socket)
    , m_myUserId(myUserId)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);

    m_listWidget = new QListWidget();
    m_listWidget->setStyleSheet(
        "QListWidget { border: none; background: transparent; color: #202a31; outline: none; }"
        "QListWidget::item { padding: 6px 4px; border-bottom: 1px solid #edf1f4; color: #202a31; }"
        "QListWidget::item:hover { background: #f5f6f8; border-radius: 6px; }"
        "QListWidget::item:selected { background: #e7f8ef; border-radius: 6px; color: #202a31; }"
    );
    layout->addWidget(m_listWidget);

    connect(m_socket, &WeChatSocket::frameReceived, this, &GroupListWidget::onFrameReceived);

    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        int groupId = item->data(Qt::UserRole).toInt();
        QString name = item->data(Qt::UserRole + 1).toString();
        if (groupId > 0) emit openGroupChat(groupId, name);
    });

    refreshGroups();
}

void GroupListWidget::refreshGroups()
{
    QJsonObject msg = MessageBuilder::buildGroupList();
    msg["seq"] = static_cast<int>(m_socket->nextSeq());
    m_socket->sendFrame(FrameType::REQUEST, msg);
}

void GroupListWidget::onFrameReceived(const Frame& frame)
{
    QString type = frame.payload.value("type").toString();

    if (type == MsgType::GROUP_LIST_RES) {
        GroupListResponse r = MessageBuilder::parseGroupListResponse(frame.payload);
        m_listWidget->clear();

        if (r.groups.empty()) {
            auto* item = new QListWidgetItem();
            item->setSizeHint(QSize(0, 178));
            auto* emptyWidget = new QWidget();
            emptyWidget->setStyleSheet(
                "QWidget { background: #ffffff; border: 1px dashed #cbd5dc; border-radius: 8px; }"
                "QLabel { background: transparent; border: none; }"
            );
            auto* emptyLayout = new QVBoxLayout(emptyWidget);
            emptyLayout->setAlignment(Qt::AlignCenter);
            emptyLayout->setSpacing(8);
            emptyLayout->setContentsMargins(18, 18, 18, 18);

            auto* icon = new QLabel("👥");
            icon->setAlignment(Qt::AlignCenter);
            icon->setStyleSheet("font-size: 40px; color: #a6b2ba; background: transparent;");
            emptyLayout->addWidget(icon);

            auto* emptyTitle = new QLabel("暂无群聊");
            emptyTitle->setAlignment(Qt::AlignCenter);
            emptyTitle->setStyleSheet("font-size: 16px; font-weight: 900; color: #4f5c65; background: transparent;");
            emptyLayout->addWidget(emptyTitle);

            auto* emptyDesc = new QLabel("点击上方按钮，创建群聊或申请加入。");
            emptyDesc->setAlignment(Qt::AlignCenter);
            emptyDesc->setWordWrap(true);
            emptyDesc->setStyleSheet("font-size: 14px; color: #6f7f8b; font-weight: 700; background: transparent;");
            emptyLayout->addWidget(emptyDesc);

            m_listWidget->addItem(item);
            m_listWidget->setItemWidget(item, emptyWidget);
        } else {
            for (const auto& g : r.groups) {
                auto* item = new QListWidgetItem();
                item->setSizeHint(QSize(0, 60));
                item->setData(Qt::UserRole, g.groupId);
                item->setData(Qt::UserRole + 1, g.groupName);

                auto* widget = new QWidget();
                widget->setStyleSheet("background: transparent;");
                auto* hbox = new QHBoxLayout(widget);
                hbox->setContentsMargins(4, 6, 4, 6);
                hbox->setSpacing(10);

                auto* avatar = new QLabel();
                avatar->setFixedSize(36, 36);
                avatar->setAlignment(Qt::AlignCenter);
                avatar->setStyleSheet(
                    "background: #e9f8ef; border-radius: 8px; font-size: 18px; color: #158a57; border: 1px solid #d9efe3;"
                );
                avatar->setText("👥");
                hbox->addWidget(avatar);

                auto* nameLabel = new QLabel(g.groupName);
                nameLabel->setStyleSheet("font-size: 15px; font-weight: 900; color: #202a31; background: transparent;");
                hbox->addWidget(nameLabel, 1);

                auto* idLabel = new QLabel(QString("ID:%1").arg(g.groupId));
                idLabel->setStyleSheet("font-size: 13px; color: #6f7f8b; font-weight: 700; background: transparent;");
                hbox->addWidget(idLabel);

                m_listWidget->addItem(item);
                m_listWidget->setItemWidget(item, widget);
            }
        }
    }
}
