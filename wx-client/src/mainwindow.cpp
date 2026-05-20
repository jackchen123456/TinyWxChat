#include "mainwindow.h"
#include "networkclient.h"
#include <QApplication>
#include <QSplitter>
#include <QScrollBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("TinyWeChat");
    resize(420, 680);

    client_ = new NetworkClient(this);
    connectSignals();

    stack_ = new QStackedWidget(this);
    setCentralWidget(stack_);

    setupLoginPage();
    setupRegisterPage();
    setupContactPage();
    setupGroupPage();
    setupChatPage();

    stack_->setCurrentWidget(loginPage_);

    // Connect to server
    client_->connectToServer("127.0.0.1", 9090);
}

// ── Signal wiring ───────────────────────────────────────

void MainWindow::connectSignals()
{
    connect(client_, &NetworkClient::loginResult,
            this, &MainWindow::onLoginResult);
    connect(client_, &NetworkClient::messageReceived,
            this, &MainWindow::onMessageReceived);
    connect(client_, &NetworkClient::groupMessageReceived,
            this, &MainWindow::onGroupMessageReceived);
    connect(client_, &NetworkClient::responseReceived,
            this, [this](const QString& type, int, const QJsonObject& body) {
        if (type == MsgType::SEND_RES) {
            // message sent ack — handled in onSendChat
        } else if (type == MsgType::HISTORY_RES) {
            auto msgs = body.value("messages").toArray();
            for (const auto& m : msgs) {
                auto obj = m.toObject();
                QString nick = obj["from_nickname"].toString();
                QString prefix = (obj["from_user_id"].toInt() == myUserId_)
                    ? "我" : nick;
                chatHistory_->append(
                    QString("[%1] %2: %3")
                        .arg(QDateTime::fromSecsSinceEpoch(obj["timestamp"].toInt())
                             .toString("HH:mm"))
                        .arg(prefix)
                        .arg(obj["content"].toString()));
            }
        } else if (type == MsgType::GROUP_HISTORY_RES) {
            auto msgs = body.value("messages").toArray();
            for (const auto& m : msgs) {
                auto obj = m.toObject();
                chatHistory_->append(
                    QString("[%1] %2: %3")
                        .arg(QDateTime::fromSecsSinceEpoch(obj["timestamp"].toInt())
                             .toString("HH:mm"))
                        .arg(obj["from_nickname"].toString())
                        .arg(obj["content"].toString()));
            }
        }
    });
    connect(client_, &NetworkClient::notificationReceived,
            this, [this](const QString& type, const QJsonObject& body) {
        if (type == MsgType::FRIEND_NOTIFY) {
            QMessageBox::information(this, "好友申请",
                QString("%1 请求添加你为好友").arg(body["from_nickname"].toString()));
        }
        refreshPendingRequests();
    });
}

// ── Login ───────────────────────────────────────────────

void MainWindow::setupLoginPage()
{
    loginPage_ = new QWidget;
    auto* lay = new QVBoxLayout(loginPage_);
    lay->addStretch();

    auto* title = new QLabel("<h1>TinyWeChat</h1>");
    title->setAlignment(Qt::AlignCenter);
    lay->addWidget(title);

    loginUser_ = new QLineEdit; loginUser_->setPlaceholderText("用户名");
    loginPass_ = new QLineEdit; loginPass_->setPlaceholderText("密码");
    loginPass_->setEchoMode(QLineEdit::Password);
    loginBtn_  = new QPushButton("登 录");
    toRegisterBtn_ = new QPushButton("注册新账号");

    lay->addWidget(loginUser_);
    lay->addWidget(loginPass_);
    lay->addWidget(loginBtn_);
    lay->addWidget(toRegisterBtn_);
    lay->addStretch();

    connect(loginBtn_, &QPushButton::clicked, this, &MainWindow::onLogin);
    connect(toRegisterBtn_, &QPushButton::clicked, this, &MainWindow::goToRegister);
    connect(loginPass_, &QLineEdit::returnPressed, this, &MainWindow::onLogin);

    stack_->addWidget(loginPage_);
}

void MainWindow::onLogin()
{
    QString user = loginUser_->text().trimmed();
    QString pass = loginPass_->text();
    if (user.isEmpty() || pass.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入用户名和密码");
        return;
    }
    QJsonObject body;
    body["username"] = user;
    body["password"] = pass;
    client_->sendRequest(MsgType::LOGIN, body);
}

void MainWindow::onLoginResult(int code, int userId, const QString& nickname)
{
    if (code == 0) {
        myUserId_   = userId;
        myNickname_ = nickname;
        setWindowTitle(QString("TinyWeChat — %1").arg(nickname));
        goToContacts();
    } else {
        QMessageBox::warning(this, "登录失败", "用户名或密码错误");
    }
}

// ── Register ────────────────────────────────────────────

void MainWindow::goToRegister() { stack_->setCurrentWidget(registerPage_); }
void MainWindow::goToLogin()    { stack_->setCurrentWidget(loginPage_); }

void MainWindow::setupRegisterPage()
{
    registerPage_ = new QWidget;
    auto* lay = new QVBoxLayout(registerPage_);
    lay->addStretch();

    auto* title = new QLabel("<h1>注册</h1>");
    title->setAlignment(Qt::AlignCenter);
    lay->addWidget(title);

    regUser_ = new QLineEdit; regUser_->setPlaceholderText("用户名（最长32字节）");
    regPass_ = new QLineEdit; regPass_->setPlaceholderText("密码（最少6位）");
    regPass_->setEchoMode(QLineEdit::Password);
    regNick_ = new QLineEdit; regNick_->setPlaceholderText("昵称（可选）");
    regBtn_  = new QPushButton("注 册");
    toLoginBtn_ = new QPushButton("返回登录");

    lay->addWidget(regUser_);
    lay->addWidget(regPass_);
    lay->addWidget(regNick_);
    lay->addWidget(regBtn_);
    lay->addWidget(toLoginBtn_);
    lay->addStretch();

    connect(regBtn_, &QPushButton::clicked, this, &MainWindow::onRegister);
    connect(toLoginBtn_, &QPushButton::clicked, this, &MainWindow::goToLogin);

    stack_->addWidget(registerPage_);
}

void MainWindow::onRegister()
{
    QString user = regUser_->text().trimmed();
    QString pass = regPass_->text();
    QString nick = regNick_->text().trimmed();
    if (user.isEmpty() || pass.isEmpty()) {
        QMessageBox::warning(this, "提示", "用户名和密码不能为空");
        return;
    }
    QJsonObject body;
    body["username"] = user;
    body["password"] = pass;
    body["nickname"] = nick.isEmpty() ? user : nick;
    int seq = client_->sendRequest(MsgType::REGISTER, body);

    // Temporary: wire a response handler
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(client_, &NetworkClient::responseReceived,
                    this, [this, conn](const QString& type, int, const QJsonObject& body) {
        if (type == MsgType::REGISTER_RES) {
            disconnect(*conn);
            int code = body["code"].toInt();
            if (code == 0) {
                QMessageBox::information(this, "注册成功",
                    QString("注册成功！用户ID: %1").arg(body["user_id"].toInt()));
                goToLogin();
            } else {
                QMessageBox::warning(this, "注册失败",
                    body.value("message").toString("用户名已被注册"));
            }
        }
    });
}

// ── Contacts ────────────────────────────────────────────

void MainWindow::setupContactPage()
{
    contactPage_ = new QWidget;
    auto* lay = new QVBoxLayout(contactPage_);

    auto* title = new QLabel("<h2>好友</h2>");
    lay->addWidget(title);

    friendList_ = new QListWidget;
    lay->addWidget(friendList_);

    // Pending requests
    auto* pendingGroup = new QGroupBox("好友申请");
    auto* pLay = new QVBoxLayout(pendingGroup);
    pendingList_ = new QListWidget;
    pLay->addWidget(pendingList_);
    auto* acceptBtn = new QPushButton("同意选中");
    pLay->addWidget(acceptBtn);
    lay->addWidget(pendingGroup);
    connect(acceptBtn, &QPushButton::clicked, this, &MainWindow::onAcceptFriend);

    // Add friend
    auto* addGroup = new QGroupBox("添加好友");
    auto* aLay = new QHBoxLayout(addGroup);
    addFriendUid_ = new QLineEdit; addFriendUid_->setPlaceholderText("用户ID");
    addFriendMsg_ = new QLineEdit; addFriendMsg_->setPlaceholderText("附言");
    addFriendBtn_ = new QPushButton("发送申请");
    aLay->addWidget(addFriendUid_);
    aLay->addWidget(addFriendMsg_);
    aLay->addWidget(addFriendBtn_);
    lay->addWidget(addGroup);
    connect(addFriendBtn_, &QPushButton::clicked, this, &MainWindow::onFriendRequest);

    // Bottom bar
    auto* bot = new QHBoxLayout;
    toGroupsBtn_ = new QPushButton("群聊");
    auto* refreshBtn = new QPushButton("刷新");
    bot->addWidget(refreshBtn);
    bot->addStretch();
    bot->addWidget(toGroupsBtn_);
    lay->addLayout(bot);

    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshFriendList);
    connect(toGroupsBtn_, &QPushButton::clicked, this, &MainWindow::goToGroups);
    connect(friendList_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        int uid = item->data(Qt::UserRole).toInt();
        QString nick = item->text();
        openChatWith(uid, nick);
    });

    stack_->addWidget(contactPage_);
}

void MainWindow::goToContacts()
{
    onRefreshFriendList();
    refreshPendingRequests();
    stack_->setCurrentWidget(contactPage_);
}

void MainWindow::onRefreshFriendList()
{
    friendList_->clear();
    client_->sendRequest(MsgType::FRIEND_LIST, {});
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(client_, &NetworkClient::responseReceived,
                    this, [this, conn](const QString& type, int, const QJsonObject& body) {
        if (type == MsgType::FRIEND_LIST_RES) {
            disconnect(*conn);
            friendList_->clear();
            auto friends = body["friends"].toArray();
            for (const auto& f : friends) {
                auto obj = f.toObject();
                auto* item = new QListWidgetItem(obj["nickname"].toString());
                item->setData(Qt::UserRole, obj["user_id"].toInt());
                friendList_->addItem(item);
            }
        }
    });
}

void MainWindow::refreshPendingRequests()
{
    // pendingList_ might not be initialized if contactPage not shown
    if (!pendingList_) return;
    client_->sendRequest(MsgType::FRIEND_PENDING, {});
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(client_, &NetworkClient::responseReceived,
                    this, [this, conn](const QString& type, int, const QJsonObject& body) {
        if (type == MsgType::FRIEND_PENDING_RES) {
            disconnect(*conn);
            pendingList_->clear();
            auto reqs = body["requests"].toArray();
            for (const auto& r : reqs) {
                auto obj = r.toObject();
                auto* item = new QListWidgetItem(
                    QString("%1: %2").arg(obj["from_nickname"].toString())
                                     .arg(obj["message"].toString()));
                item->setData(Qt::UserRole, obj["request_id"].toInt());
                pendingList_->addItem(item);
            }
        }
    });
}

void MainWindow::onFriendRequest()
{
    bool ok;
    int uid = addFriendUid_->text().toInt(&ok);
    if (!ok || uid == 0) {
        QMessageBox::warning(this, "提示", "请输入有效的用户ID");
        return;
    }
    QJsonObject body;
    body["to_user_id"] = uid;
    body["message"]    = addFriendMsg_->text();
    client_->sendRequest(MsgType::FRIEND_REQUEST, body);
    addFriendUid_->clear();
    addFriendMsg_->clear();
}

void MainWindow::onAcceptFriend()
{
    auto* item = pendingList_->currentItem();
    if (!item) {
        QMessageBox::information(this, "提示", "请先选择一条申请");
        return;
    }
    int reqId = item->data(Qt::UserRole).toInt();
    QJsonObject body;
    body["request_id"] = reqId;
    body["action"]     = "accept";
    client_->sendRequest(MsgType::FRIEND_HANDLE, body);
    refreshPendingRequests();
    onRefreshFriendList();
}

// ── Groups ──────────────────────────────────────────────

void MainWindow::goToGroups()
{
    onRefreshGroupList();
    stack_->setCurrentWidget(groupPage_);
}

void MainWindow::setupGroupPage()
{
    groupPage_ = new QWidget;
    auto* lay = new QVBoxLayout(groupPage_);

    auto* title = new QLabel("<h2>群聊</h2>");
    lay->addWidget(title);

    groupList_ = new QListWidget;
    lay->addWidget(groupList_);

    // Create group
    auto* createGroup = new QGroupBox("创建群聊");
    auto* cLay = new QHBoxLayout(createGroup);
    groupNameEdit_ = new QLineEdit; groupNameEdit_->setPlaceholderText("群名称");
    createGroupBtn_ = new QPushButton("创建");
    cLay->addWidget(groupNameEdit_);
    cLay->addWidget(createGroupBtn_);
    lay->addWidget(createGroup);
    connect(createGroupBtn_, &QPushButton::clicked, this, &MainWindow::onCreateGroup);

    // Join group
    auto* joinGroup = new QGroupBox("加入群聊");
    auto* jLay = new QHBoxLayout(joinGroup);
    joinGroupId_ = new QLineEdit; joinGroupId_->setPlaceholderText("群ID");
    joinGroupBtn_ = new QPushButton("加入");
    jLay->addWidget(joinGroupId_);
    jLay->addWidget(joinGroupBtn_);
    lay->addWidget(joinGroup);
    connect(joinGroupBtn_, &QPushButton::clicked, this, &MainWindow::onJoinGroup);

    // Open group chat
    openGroupBtn_ = new QPushButton("打开选中群聊");
    lay->addWidget(openGroupBtn_);
    connect(openGroupBtn_, &QPushButton::clicked, this, &MainWindow::onOpenGroupChat);

    // Bottom
    auto* bot = new QHBoxLayout;
    toContactsBtn_ = new QPushButton("好友");
    auto* refreshBtn = new QPushButton("刷新");
    bot->addWidget(refreshBtn);
    bot->addStretch();
    bot->addWidget(toContactsBtn_);
    lay->addLayout(bot);

    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshGroupList);
    connect(toContactsBtn_, &QPushButton::clicked, this, &MainWindow::goToContacts);

    stack_->addWidget(groupPage_);
}

void MainWindow::onRefreshGroupList()
{
    client_->sendRequest(MsgType::GROUP_LIST, {});
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(client_, &NetworkClient::responseReceived,
                    this, [this, conn](const QString& type, int, const QJsonObject& body) {
        if (type == MsgType::GROUP_LIST_RES) {
            disconnect(*conn);
            groupList_->clear();
            auto groups = body["groups"].toArray();
            for (const auto& g : groups) {
                auto obj = g.toObject();
                int gid = obj["group_id"].toInt();
                QString gname = obj["group_name"].toString();
                auto* item = new QListWidgetItem(
                    QString("群%1 (%2)").arg(gid).arg(gname));
                item->setData(Qt::UserRole, gid);
                groupList_->addItem(item);
            }
        }
    });
}

void MainWindow::onCreateGroup()
{
    QString name = groupNameEdit_->text().trimmed();
    if (name.isEmpty()) return;
    QJsonObject body;
    body["name"] = name;
    client_->sendRequest(MsgType::GROUP_CREATE, body);
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(client_, &NetworkClient::responseReceived,
                    this, [this, conn](const QString& type, int, const QJsonObject& body) {
        if (type == MsgType::GROUP_CREATE_RES) {
            disconnect(*conn);
            int gid = body["group_id"].toInt();
            QString gname = groupNameEdit_->text();  // cache before clear
            groupNameEdit_->clear();
            QMessageBox::information(this, "群聊创建",
                QString("群聊已创建！群ID: %1\n请把群ID告诉好友让他们加入").arg(gid));
            // Add to local list
            auto* item = new QListWidgetItem(
                QString("群%1 (%2)").arg(gid).arg(gname));
            item->setData(Qt::UserRole, gid);
            groupList_->addItem(item);
        }
    });
}

void MainWindow::onJoinGroup()
{
    bool ok;
    int gid = joinGroupId_->text().toInt(&ok);
    if (!ok || gid == 0) return;
    QJsonObject body;
    body["group_id"] = gid;
    client_->sendRequest(MsgType::GROUP_JOIN, body);
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(client_, &NetworkClient::responseReceived,
                    this, [this, gid, conn](const QString& type, int, const QJsonObject& body) {
        if (type == MsgType::GROUP_JOIN_RES) {
            disconnect(*conn);
            if (body["code"].toInt() == 0) {
                auto* item = new QListWidgetItem(QString("群%1").arg(gid));
                item->setData(Qt::UserRole, gid);
                groupList_->addItem(item);
                joinGroupId_->clear();
            }
        }
    });
}

void MainWindow::onOpenGroupChat()
{
    auto* item = groupList_->currentItem();
    if (!item) return;
    int gid = item->data(Qt::UserRole).toInt();
    chatGroupId_ = gid;
    chatTargetUid_ = 0;
    chatTitle_->setText(QString("群聊 %1").arg(gid));
    chatHistory_->clear();
    chatBackBtn_->setText("← 群列表");
    requestGroupHistory(gid);
    stack_->setCurrentWidget(chatPage_);
}

void MainWindow::onSendGroupChat()
{
    QString text = chatInput_->text().trimmed();
    if (text.isEmpty() || chatGroupId_ == 0) return;
    QJsonObject body;
    body["group_id"] = chatGroupId_;
    body["content"]  = text;
    body["msg_type"] = 1;
    client_->sendRequest(MsgType::GROUP_SEND, body);
    chatInput_->clear();
}

void MainWindow::requestGroupHistory(int groupId)
{
    QJsonObject body;
    body["group_id"] = groupId;
    body["limit"]    = 50;
    client_->sendRequest(MsgType::GROUP_HISTORY, body);
}

// ── Chat ────────────────────────────────────────────────

void MainWindow::setupChatPage()
{
    chatPage_ = new QWidget;
    auto* lay = new QVBoxLayout(chatPage_);

    // Top bar
    auto* top = new QHBoxLayout;
    chatBackBtn_ = new QPushButton("← 好友");
    chatTitle_   = new QLabel("<b>聊天</b>");
    top->addWidget(chatBackBtn_);
    top->addWidget(chatTitle_, 1);
    lay->addLayout(top);

    chatHistory_ = new QTextEdit;
    chatHistory_->setReadOnly(true);
    lay->addWidget(chatHistory_);

    auto* inputLay = new QHBoxLayout;
    chatInput_  = new QLineEdit;
    chatInput_->setPlaceholderText("输入消息...");
    chatSendBtn_ = new QPushButton("发送");
    inputLay->addWidget(chatInput_, 1);
    inputLay->addWidget(chatSendBtn_);
    lay->addLayout(inputLay);

    connect(chatSendBtn_, &QPushButton::clicked, this, &MainWindow::onSendChat);
    connect(chatInput_, &QLineEdit::returnPressed, this, &MainWindow::onSendChat);
    connect(chatBackBtn_, &QPushButton::clicked, this, [this]() {
        if (chatGroupId_ > 0) {
            chatGroupId_ = 0;
            goToGroups();
        } else {
            goToContacts();
        }
    });

    stack_->addWidget(chatPage_);
}

void MainWindow::openChatWith(int uid, const QString& nick)
{
    chatTargetUid_ = uid;
    chatGroupId_   = 0;
    chatTitle_->setText(QString("与 %1 聊天").arg(nick));
    chatBackBtn_->setText("← 好友");
    chatHistory_->clear();
    requestHistory(uid);
}

void MainWindow::onSendChat()
{
    QString text = chatInput_->text().trimmed();
    if (text.isEmpty()) return;

    if (chatGroupId_ > 0) {
        onSendGroupChat();
        return;
    }

    if (chatTargetUid_ == 0) return;

    QJsonObject body;
    body["to_user_id"] = chatTargetUid_;
    body["content"]    = text;
    body["msg_type"]   = 1;
    client_->sendRequest(MsgType::SEND, body);

    // Show locally
    chatHistory_->append(
        QString("[%1] 我: %2")
            .arg(QTime::currentTime().toString("HH:mm"))
            .arg(text));
    chatInput_->clear();
}

void MainWindow::onMessageReceived(int fromUid, const QString& fromNick,
                                    int /*msgId*/, const QString& content, int /*timestamp*/)
{
    // If this is the chat we're currently viewing, show it
    if (fromUid == chatTargetUid_ && chatGroupId_ == 0) {
        chatHistory_->append(
            QString("[%1] %2: %3")
                .arg(QTime::currentTime().toString("HH:mm"))
                .arg(fromNick)
                .arg(content));
    } else {
        // Message from someone else — status bar notification
        statusBar()->showMessage(
            QString("%1: %2").arg(fromNick).arg(content), 5000);
    }
}

void MainWindow::onGroupMessageReceived(int groupId, int /*fromUid*/,
    const QString& fromNick, int /*msgSeq*/, const QString& content, int /*timestamp*/)
{
    // Only show if we're currently viewing this group
    if (groupId == chatGroupId_) {
        chatHistory_->append(
            QString("[%1] %2: %3")
                .arg(QTime::currentTime().toString("HH:mm"))
                .arg(fromNick)
                .arg(content));
    } else {
        // Notification for other groups
        statusBar()->showMessage(
            QString("[群%1] %2: %3").arg(groupId).arg(fromNick).arg(content), 5000);
    }
}

void MainWindow::requestHistory(int withUid)
{
    QJsonObject body;
    body["with_user_id"] = withUid;
    body["limit"]        = 50;
    client_->sendRequest(MsgType::HISTORY, body);
}
