#include "MainHubWidget.h"
#include "AvatarSelectDialog.h"
#include <QVBoxLayout>
#include <QDebug>

MainHubWidget::MainHubWidget(WeChatSocket* socket, int myUserId,
                             const QString& myNickname, QWidget* parent)
    : QWidget(parent)
    , m_socket(socket)
    , m_myUserId(myUserId)
    , m_myNickname(myNickname)
    , m_avatarColor("#3498db")
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── 顶部栏：头像 + 用户名 ──
    auto* headerBar = new QHBoxLayout();
    headerBar->setContentsMargins(8, 6, 8, 6);

    m_myAvatar = new AvatarLabel();
    m_myAvatar->setNickname(myNickname);
    m_myAvatar->setFixedSize(32, 32);
    m_myAvatar->setCursor(Qt::PointingHandCursor);
    m_myAvatar->setToolTip("点击更换头像");
    headerBar->addWidget(m_myAvatar);

    auto* nameLabel = new QLabel(QString("<b>%1</b> (ID:%2)").arg(myNickname).arg(myUserId));
    nameLabel->setTextFormat(Qt::RichText);
    headerBar->addWidget(nameLabel);
    headerBar->addStretch();

    mainLayout->addLayout(headerBar);

    // ── 标签栏 ──
    auto* tabBar = new QHBoxLayout();
    tabBar->setContentsMargins(0, 0, 0, 0);
    tabBar->setSpacing(0);

    QStringList tabs = {"聊天", "好友", "群聊"};
    for (int i = 0; i < tabs.size(); ++i) {
        auto* btn = new QPushButton(tabs[i]);
        btn->setCheckable(true);
        btn->setMinimumHeight(36);
        btn->setStyleSheet(
            "QPushButton { border: none; border-bottom: 2px solid transparent; "
            "font-size: 15px; padding: 6px 20px; background: transparent; }"
            "QPushButton:checked { border-bottom: 2px solid #07c160; color: #07c160; font-weight: bold; }"
        );
        connect(btn, &QPushButton::clicked, this, [this, i]() { switchTab(i); });
        tabBar->addWidget(btn, 1);
        m_tabBtns.append(btn);
    }
    mainLayout->addLayout(tabBar);

    // ── 内容区 ──
    m_stack = new QStackedWidget();

    m_convList = new ConversationListWidget(m_socket);
    m_stack->addWidget(m_convList);

    m_friendList = new FriendListWidget(m_socket, m_myUserId);
    m_stack->addWidget(m_friendList);

    m_groupList = new GroupListWidget(m_socket, m_myUserId);
    m_stack->addWidget(m_groupList);

    mainLayout->addWidget(m_stack, 1);

    // ── 信号 ──
    connect(m_convList, &ConversationListWidget::conversationClicked,
            this, &MainHubWidget::openChat);
    connect(m_friendList, &FriendListWidget::startChat,
            this, [this](int userId, const QString& nickname) {
        emit openChat(userId, nickname);
    });
    connect(m_groupList, &GroupListWidget::openGroupChat,
            this, &MainHubWidget::openGroupChat);

    connect(m_myAvatar, &AvatarLabel::clicked, this, &MainHubWidget::onChangeAvatar);

    // §4: 好友列表加载完成后同步到会话列表
    connect(m_friendList, &FriendListWidget::friendsLoaded,
            this, &MainHubWidget::onFriendsLoaded);

    switchTab(0);

    // 登录后自动拉取好友列表（触发 onFriendsLoaded → 同步会话列表）
    m_friendList->refreshFriends();
}

void MainHubWidget::switchTab(int index)
{
    if (index < 0 || index >= m_tabBtns.size()) return;
    setActiveTab(index);
    m_stack->setCurrentIndex(index);

    if (index == 1) {
        m_friendList->refreshFriends();
        m_friendList->refreshPending();
    }
}

void MainHubWidget::setActiveTab(int index)
{
    for (int i = 0; i < m_tabBtns.size(); ++i) {
        m_tabBtns[i]->setChecked(i == index);
    }
}

void MainHubWidget::onChangeAvatar()
{
    AvatarSelectDialog dlg(m_myNickname, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_avatarColor = dlg.selectedColor();
        m_myAvatar->setAvatarColor(m_avatarColor);
        qDebug() << "[MainHub] Avatar color changed to" << m_avatarColor.name();
    }
}

void MainHubWidget::onFriendsLoaded(const QList<QPair<int, QString>>& friends)
{
    // §4: 会话列表展示最近会话（从好友列表初始化）
    if (friends.isEmpty()) return;

    QList<ConversationItem> items;
    for (const auto& f : friends) {
        ConversationItem c;
        c.userId    = f.first;
        c.nickname  = f.second;
        c.lastMsg   = "";
        c.timestamp = 0;
        items.append(c);
    }
    m_convList->setConversations(items);
    qDebug() << "[MainHub] Synced" << friends.size() << "friends to conversation list";
}
