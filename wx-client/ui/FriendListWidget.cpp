#include "FriendListWidget.h"
#include "AvatarLabel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QDebug>

// ============================================================
//  AddFriendDialog (unchanged)
// ============================================================

AddFriendDialog::AddFriendDialog(WeChatSocket* socket, QWidget* parent)
    : QDialog(parent)
    , m_socket(socket)
{
    setWindowTitle("添加好友");
    setMinimumWidth(320);
    setStyleSheet(
        "QDialog { background: #ffffff; color: #202a31; }"
        "QLabel { color: #33414a; font-size: 14px; font-weight: 800; }"
        "QLineEdit { border: 1px solid #dde3e8; border-radius: 8px;"
        "  padding: 0 12px; min-height: 38px; font-size: 15px; color: #202a31;"
        "  background: #ffffff; selection-background-color: #27c277; }"
        "QLineEdit:focus { border-color: rgba(39,194,119,0.65); }"
        "QPushButton { min-height: 34px; border-radius: 7px; padding: 0 14px;"
        "  font-size: 14px; font-weight: 800; color: #202a31; background: #f5f8fa;"
        "  border: 1px solid #dde3e8; }"
        "QPushButton:hover { background: #eef3f6; }"
    );
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    layout->addWidget(new QLabel("对方用户ID："));
    m_userIdEdit = new QLineEdit();
    m_userIdEdit->setPlaceholderText("输入用户ID（数字）");
    layout->addWidget(m_userIdEdit);

    layout->addWidget(new QLabel("验证消息（可选）："));
    m_messageEdit = new QLineEdit();
    m_messageEdit->setPlaceholderText("你好，加个好友吧");
    layout->addWidget(m_messageEdit);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: #c93d3d; font-size: 14px; font-weight: 700;");
    layout->addWidget(m_statusLabel);

    auto* btnLayout = new QHBoxLayout();
    auto* cancelBtn = new QPushButton("取消");
    auto* sendBtn = new QPushButton("发送申请");
    sendBtn->setStyleSheet(
        "QPushButton { background: #27c277; color: white; border: none;"
        "  border-radius: 7px; padding: 0 16px; font-weight: 900; font-size: 14px; }"
        "QPushButton:hover { background: #158a57; }"
    );
    btnLayout->addWidget(cancelBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(sendBtn);
    layout->addLayout(btnLayout);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(sendBtn, &QPushButton::clicked, this, &AddFriendDialog::onSendClicked);
}

void AddFriendDialog::onSendClicked()
{
    QString idText = m_userIdEdit->text().trimmed();
    if (idText.isEmpty()) { m_statusLabel->setText("请输入用户ID"); return; }
    bool ok = false;
    int userId = idText.toInt(&ok);
    if (!ok || userId <= 0) { m_statusLabel->setText("用户ID必须为正整数"); return; }

    QJsonObject msg = MessageBuilder::buildFriendRequest(userId, m_messageEdit->text().trimmed());
    msg["seq"] = static_cast<int>(m_socket->nextSeq());
    if (m_socket->sendFrame(FrameType::REQUEST, msg)) {
        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(m_socket, &WeChatSocket::frameReceived, this,
                        [this, conn](const Frame& frame) {
            QString type = frame.payload.value("type").toString();
            if (type == MsgType::FRIEND_REQUEST_RES) {
                FriendRequestResponse r = MessageBuilder::parseFriendRequestResponse(frame.payload);
                if (r.ok) {
                    m_statusLabel->setStyleSheet("color: #158a57; font-size: 14px; font-weight: 700;");
                    m_statusLabel->setText("申请已发送！");
                    QTimer::singleShot(800, this, &QDialog::accept);
                } else {
                    m_statusLabel->setStyleSheet("color: #c93d3d; font-size: 14px; font-weight: 700;");
                    QString hint;
                    switch (r.code) {
                    case 200: hint = "不能向自己发送好友申请"; break;
                    case 300: hint = "目标用户不存在"; break;
                    case 311: hint = "你们已经是好友了"; break;
                    case 312: hint = "已有未处理的好友申请，请勿重复发送"; break;
                    default: hint = r.errorMsg.isEmpty() ? QString("申请失败 (code=%1)").arg(r.code) : r.errorMsg; break;
                    }
                    m_statusLabel->setText(hint);
                }
                disconnect(*conn);
            }
        });
    } else {
        m_statusLabel->setText("发送失败，连接已断开");
    }
}

// ============================================================
//  FriendListWidget — 精简版：只有列表，不含标题/搜索/按钮
// ============================================================

FriendListWidget::FriendListWidget(WeChatSocket* socket, int myUserId, QWidget* parent)
    : QWidget(parent)
    , m_socket(socket)
    , m_myUserId(myUserId)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);

    // 好友列表
    m_friendList = new QListWidget();
    m_friendList->setStyleSheet(
        "QListWidget { border: none; background: transparent; color: #202a31; outline: none; }"
        "QListWidget::item { padding: 6px 4px; border-bottom: 1px solid #edf1f4; color: #202a31; }"
        "QListWidget::item:hover { background: #f5f6f8; border-radius: 6px; }"
        "QListWidget::item:selected { background: #e7f8ef; border-radius: 6px; color: #202a31; }"
    );
    layout->addWidget(m_friendList, 3);

    // 待处理申请标签
    auto* pendingLabel = new QLabel("待处理的好友申请 (0)");
    pendingLabel->setObjectName("pendingLabel");
    pendingLabel->setStyleSheet("font-size: 15px; font-weight: 900; color: #3e4b54; background: transparent;");
    layout->addWidget(pendingLabel);

    // 待处理申请列表
    m_pendingList = new QListWidget();
    m_pendingList->setFixedHeight(100);
    m_pendingList->setStyleSheet(
        "QListWidget { border: 1px solid #f4c430; border-radius: 8px; background: #fffaf0;"
        "  color: #5c4a18; outline: none; font-size: 14px; font-weight: 800; }"
        "QListWidget::item { padding: 8px; border-bottom: 1px solid #f0d060; color: #5c4a18; }"
        "QListWidget::item:selected { background: #fff0bf; color: #5c4a18; }"
    );
    layout->addWidget(m_pendingList);

    connect(m_socket, &WeChatSocket::frameReceived, this, &FriendListWidget::onFrameReceived);

    connect(m_friendList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        int userId = item->data(Qt::UserRole).toInt();
        QString name = item->data(Qt::UserRole + 1).toString();
        if (userId > 0) emit startChat(userId, name);
    });

    connect(m_pendingList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        int reqId = item->data(Qt::UserRole).toInt();
        QJsonObject msg = MessageBuilder::buildFriendHandle(reqId, "accept");
        msg["seq"] = static_cast<int>(m_socket->nextSeq());
        m_socket->sendFrame(FrameType::REQUEST, msg);
    });
}

void FriendListWidget::refreshFriends()
{
    QJsonObject msg = MessageBuilder::buildFriendList();
    msg["seq"] = static_cast<int>(m_socket->nextSeq());
    m_socket->sendFrame(FrameType::REQUEST, msg);
}

void FriendListWidget::refreshPending()
{
    QJsonObject msg = MessageBuilder::buildFriendPending();
    msg["seq"] = static_cast<int>(m_socket->nextSeq());
    m_socket->sendFrame(FrameType::REQUEST, msg);
}

void FriendListWidget::onFrameReceived(const Frame& frame)
{
    QString type = frame.payload.value("type").toString();

    if (type == MsgType::FRIEND_LIST_RES) {
        FriendListResponse r = MessageBuilder::parseFriendListResponse(frame.payload);
        m_friendList->clear();

        QList<QPair<int, QString>> friendPairs;

        if (r.friends.empty()) {
            // 空状态卡片
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

            auto* icon = new QLabel("👤");
            icon->setAlignment(Qt::AlignCenter);
            icon->setStyleSheet("font-size: 40px; color: #a6b2ba; background: transparent;");
            emptyLayout->addWidget(icon);

            auto* emptyTitle = new QLabel("暂无好友");
            emptyTitle->setAlignment(Qt::AlignCenter);
            emptyTitle->setStyleSheet("font-size: 16px; font-weight: 900; color: #4f5c65; background: transparent;");
            emptyLayout->addWidget(emptyTitle);

            auto* emptyDesc = new QLabel("点击上方按钮，发送好友申请后即可开始聊天。");
            emptyDesc->setAlignment(Qt::AlignCenter);
            emptyDesc->setWordWrap(true);
            emptyDesc->setStyleSheet("font-size: 14px; color: #6f7f8b; font-weight: 700; background: transparent;");
            emptyLayout->addWidget(emptyDesc);

            m_friendList->addItem(item);
            m_friendList->setItemWidget(item, emptyWidget);
        } else {
            for (const auto& f : r.friends) {
                friendPairs.append({f.userId, f.nickname});

                auto* item = new QListWidgetItem();
                item->setSizeHint(QSize(0, 50));
                item->setData(Qt::UserRole, f.userId);
                item->setData(Qt::UserRole + 1, f.nickname);

                auto* widget = new QWidget();
                widget->setStyleSheet("background: transparent;");
                auto* hbox = new QHBoxLayout(widget);
                hbox->setContentsMargins(4, 6, 4, 6);
                hbox->setSpacing(10);

                auto* avatar = new AvatarLabel();
                avatar->setNickname(f.nickname);
                avatar->setFixedSize(36, 36);
                hbox->addWidget(avatar);

                auto* nameLabel = new QLabel(f.nickname);
                nameLabel->setStyleSheet("font-size: 15px; font-weight: 900; color: #202a31; background: transparent;");
                hbox->addWidget(nameLabel, 1);

                auto* idLabel = new QLabel(QString("ID:%1").arg(f.userId));
                idLabel->setStyleSheet("font-size: 13px; color: #6f7f8b; font-weight: 700; background: transparent;");
                hbox->addWidget(idLabel);

                m_friendList->addItem(item);
                m_friendList->setItemWidget(item, widget);
            }
        }

        emit friendsLoaded(friendPairs);
    }

    else if (type == MsgType::FRIEND_PENDING_RES) {
        FriendPendingResponse r = MessageBuilder::parseFriendPendingResponse(frame.payload);
        m_pendingList->clear();
        // 更新父级的待处理标签（通过 objectName 查找）
        auto* label = this->findChild<QLabel*>("pendingLabel");
        if (!label && parent()) {
            label = parent()->findChild<QLabel*>("pendingLabel");
        }
        if (label) label->setText(QString("待处理的好友申请 (%1)").arg(r.requests.size()));

        if (r.requests.empty()) {
            auto* item = new QListWidgetItem("暂无待处理申请");
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
            item->setForeground(QColor("#7b5f11"));
            m_pendingList->addItem(item);
        } else {
            for (const auto& req : r.requests) {
                QString label = QString("来自 %1 的好友申请").arg(req.fromNickname);
                if (!req.message.isEmpty()) label += QString("：%1").arg(req.message);

                auto* item = new QListWidgetItem(label);
                item->setData(Qt::UserRole, req.requestId);
                m_pendingList->addItem(item);
            }
        }
    }

    else if (type == MsgType::FRIEND_NOTIFY) {
        refreshPending();
    }

    else if (type == MsgType::FRIEND_HANDLE_RES) {
        FriendHandleResponse r = MessageBuilder::parseFriendHandleResponse(frame.payload);
        if (r.ok) {
            refreshFriends();
            refreshPending();
        }
    }
}
