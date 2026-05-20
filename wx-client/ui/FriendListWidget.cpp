#include "FriendListWidget.h"
#include "AvatarLabel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>

// ============================================================
//  AddFriendDialog
// ============================================================

AddFriendDialog::AddFriendDialog(WeChatSocket* socket, QWidget* parent)
    : QDialog(parent)
    , m_socket(socket)
{
    setWindowTitle("添加好友");
    setMinimumWidth(320);
    auto* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("对方用户ID："));
    m_userIdEdit = new QLineEdit();
    m_userIdEdit->setPlaceholderText("输入用户ID（数字）");
    layout->addWidget(m_userIdEdit);

    layout->addWidget(new QLabel("验证消息（可选）："));
    m_messageEdit = new QLineEdit();
    m_messageEdit->setPlaceholderText("你好，加个好友吧");
    layout->addWidget(m_messageEdit);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: #e74c3c;");
    layout->addWidget(m_statusLabel);

    auto* btnLayout = new QHBoxLayout();
    auto* cancelBtn = new QPushButton("取消");
    auto* sendBtn = new QPushButton("发送申请");
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
    if (idText.isEmpty()) {
        m_statusLabel->setText("请输入用户ID");
        return;
    }
    bool ok = false;
    int userId = idText.toInt(&ok);
    if (!ok || userId <= 0) {
        m_statusLabel->setText("用户ID必须为正整数");
        return;
    }

    QJsonObject msg = MessageBuilder::buildFriendRequest(userId, m_messageEdit->text().trimmed());
    msg["seq"] = static_cast<int>(m_socket->nextSeq());
    if (m_socket->sendFrame(FrameType::REQUEST, msg)) {
        // §4 Phase 2: 等待响应并显示具体错误码
        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(m_socket, &WeChatSocket::frameReceived, this,
                        [this, conn](const Frame& frame) {
            QString type = frame.payload.value("type").toString();
            if (type == "friend.request_res") {
                FriendRequestResponse r = MessageBuilder::parseFriendRequestResponse(frame.payload);
                if (r.ok) {
                    m_statusLabel->setStyleSheet("color: #27ae60;");
                    m_statusLabel->setText("申请已发送！");
                    QTimer::singleShot(800, this, &QDialog::accept);
                } else {
                    m_statusLabel->setStyleSheet("color: #e74c3c;");
                    // §4: 不能向自己/已是好友/重复申请
                    QString hint;
                    switch (r.code) {
                    case 200: hint = "不能向自己发送好友申请"; break;
                    case 300: hint = "目标用户不存在"; break;
                    case 311: hint = "你们已经是好友了"; break;
                    case 312: hint = "已有未处理的好友申请，请勿重复发送"; break;
                    default:
                        hint = r.errorMsg.isEmpty()
                            ? QString("申请失败 (code=%1)").arg(r.code)
                            : r.errorMsg;
                        break;
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
//  FriendListWidget
// ============================================================

FriendListWidget::FriendListWidget(WeChatSocket* socket, int myUserId, QWidget* parent)
    : QWidget(parent)
    , m_socket(socket)
    , m_myUserId(myUserId)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    auto* topLayout = new QHBoxLayout();
    m_friendCount = new QLabel("好友 (0)");
    m_friendCount->setStyleSheet("font-weight: bold;");
    m_addBtn = new QPushButton("+ 添加好友");
    topLayout->addWidget(m_friendCount);
    topLayout->addStretch();
    topLayout->addWidget(m_addBtn);
    layout->addLayout(topLayout);

    m_friendList = new QListWidget();
    m_friendList->setStyleSheet(
        "QListWidget { border: 1px solid #ddd; border-radius: 4px; }"
        "QListWidget::item { padding: 4px; border-bottom: 1px solid #eee; }"
        "QListWidget::item:hover { background: #f0f0f0; }"
    );
    layout->addWidget(m_friendList, 3);
    m_friendList->addItem("（点击添加好友按钮开始）");

    m_pendingLabel = new QLabel("待处理的好友申请 (0)");
    m_pendingLabel->setStyleSheet("font-weight: bold; margin-top: 8px;");
    layout->addWidget(m_pendingLabel);

    m_pendingList = new QListWidget();
    m_pendingList->setStyleSheet(
        "QListWidget { border: 1px solid #f0c040; border-radius: 4px; background: #fffbeb; }"
        "QListWidget::item { padding: 6px; border-bottom: 1px solid #f0d060; }"
    );
    layout->addWidget(m_pendingList, 1);

    connect(m_addBtn, &QPushButton::clicked, this, &FriendListWidget::onAddFriend);
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

void FriendListWidget::onAddFriend()
{
    AddFriendDialog dlg(m_socket, this);
    dlg.exec();
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

    if (type == "friend.list_res") {
        FriendListResponse r = MessageBuilder::parseFriendListResponse(frame.payload);
        m_friendList->clear();
        m_friendCount->setText(QString("好友 (%1)").arg(r.friends.size()));

        // 收集好友列表，用于同步到会话列表
        QList<QPair<int, QString>> friendPairs;

        if (r.friends.empty()) {
            m_friendList->addItem("（暂无好友，点击上方按钮添加）");
        } else {
            for (const auto& f : r.friends) {
                friendPairs.append({f.userId, f.nickname});

                auto* item = new QListWidgetItem();
                item->setSizeHint(QSize(0, 48));
                item->setData(Qt::UserRole, f.userId);
                item->setData(Qt::UserRole + 1, f.nickname);

                auto* widget = new QWidget();
                auto* hbox = new QHBoxLayout(widget);
                hbox->setContentsMargins(6, 4, 6, 4);
                hbox->setSpacing(10);

                auto* avatar = new AvatarLabel();
                avatar->setNickname(f.nickname);
                hbox->addWidget(avatar);

                auto* nameLabel = new QLabel(
                    QString("<b>%1</b> <span style='color:#888;'>(ID:%2)</span>")
                        .arg(f.nickname.toHtmlEscaped()).arg(f.userId));
                nameLabel->setTextFormat(Qt::RichText);
                hbox->addWidget(nameLabel, 1);

                auto* hintLabel = new QLabel("双击聊天");
                hintLabel->setStyleSheet("color: #aaa; font-size: 11px;");
                hbox->addWidget(hintLabel);

                m_friendList->addItem(item);
                m_friendList->setItemWidget(item, widget);
            }
        }

        // 通知外部：好友列表已加载（用于同步会话列表）
        emit friendsLoaded(friendPairs);
    }

    else if (type == "friend.pending_res") {
        FriendPendingResponse r = MessageBuilder::parseFriendPendingResponse(frame.payload);
        m_pendingList->clear();
        m_pendingLabel->setText(QString("待处理的好友申请 (%1)").arg(r.requests.size()));
        if (r.requests.empty()) {
            m_pendingList->addItem("（暂无待处理申请）");
        } else {
            for (const auto& req : r.requests) {
                QString label = QString("[%1] %2 申请加你为好友")
                    .arg(req.requestId).arg(req.fromNickname);
                if (!req.message.isEmpty())
                    label += QString("：%1").arg(req.message);
                label += "  （双击同意）";

                auto* item = new QListWidgetItem(label);
                item->setData(Qt::UserRole, req.requestId);
                m_pendingList->addItem(item);
            }
        }
    }

    else if (type == "friend.notify") {
        FriendNotify n = MessageBuilder::parseFriendNotify(frame.payload);
        qDebug() << "[FriendList] New friend request from" << n.fromNickname;
        refreshPending();
    }

    else if (type == "friend.request_res") {
        FriendRequestResponse r = MessageBuilder::parseFriendRequestResponse(frame.payload);
        qDebug() << "[FriendList] Friend request result:" << r.ok << r.code;
    }

    else if (type == "friend.handle_res") {
        FriendHandleResponse r = MessageBuilder::parseFriendHandleResponse(frame.payload);
        if (r.ok) {
            refreshFriends();
            refreshPending();
        }
    }
}
