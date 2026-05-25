#include "MainHubWidget.h"
#include "AvatarSelectDialog.h"
#include "GroupCreateDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QDebug>

MainHubWidget::MainHubWidget(WeChatSocket* socket, int myUserId,
                             const QString& myNickname, QWidget* parent)
    : QWidget(parent)
    , m_socket(socket)
    , m_myUserId(myUserId)
    , m_myNickname(myNickname)
    , m_avatarColor("#f59a23")
{
    // ══════════════════════════════════════════
    // 整体样式
    // ══════════════════════════════════════════
    setStyleSheet(
        "QWidget#hubPage { background: #fbfcfd; color: #202a31; }"
        "QLabel { color: #202a31; }"
        "QWidget#titleBar { background: #20282d; }"
        "QWidget#rail { background: #303438; border-right: 1px solid #23282c; }"
        "QWidget#middlePanel { background: #f7f8fa; }"
        "QWidget#rightPanel { background: #ffffff; }"
    );
    setObjectName("hubPage");

    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ══════════════════════════════════════════
    // 整体垂直容器：标题栏 + workspace
    // ══════════════════════════════════════════
    auto* mainVBox = new QVBoxLayout();
    mainVBox->setContentsMargins(0, 0, 0, 0);
    mainVBox->setSpacing(0);

    // ── 标题栏（62px 高）──
    auto* titleBar = new QWidget();
    titleBar->setObjectName("titleBar");
    titleBar->setFixedHeight(62);
    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(18, 0, 20, 0);
    titleLayout->setSpacing(0);

    // 左侧留空（macOS traffic lights 自动显示）
    titleLayout->addSpacing(100);

    // Logo + 标题
    auto* logoLabel = new QLabel("💬");
    logoLabel->setStyleSheet("font-size: 20px; color: #eef4f6; background: transparent;");
    titleLayout->addWidget(logoLabel);
    titleLayout->addSpacing(10);

    auto* titleLabel = new QLabel(QString("TinyWeChat — %1").arg(m_myNickname));
    titleLabel->setStyleSheet(
        "font-size: 20px; font-weight: 900; color: #eef4f6; background: transparent;"
    );
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    // 连接状态
    m_connectionPill = new QLabel();
    m_connectionPill->setTextFormat(Qt::RichText);
    m_connectionPill->setText("<span style='color:#27c277;'>●</span> 本机在线");
    m_connectionPill->setStyleSheet(
        "color: #e7edf0; font-size: 14px; font-weight: 800;"
        "background: #313c42; border-radius: 18px;"
        "padding: 8px 14px;"
    );
    titleLayout->addWidget(m_connectionPill);

    mainVBox->addWidget(titleBar);

    // ══════════════════════════════════════════
    // Workspace: rail + middle + right
    // ══════════════════════════════════════════
    auto* workspace = new QHBoxLayout();
    workspace->setContentsMargins(0, 0, 0, 0);
    workspace->setSpacing(0);

    // ══════════════════════════════════════════
    // 左侧栏 rail（88px 宽，深色）
    // ══════════════════════════════════════════
    auto* rail = new QWidget();
    rail->setObjectName("rail");
    rail->setFixedWidth(88);
    auto* railLayout = new QVBoxLayout(rail);
    railLayout->setContentsMargins(10, 26, 10, 18);
    railLayout->setSpacing(0);

    // 用户头像
    auto* profileBox = new QVBoxLayout();
    profileBox->setSpacing(10);
    profileBox->setAlignment(Qt::AlignHCenter);

    m_myAvatar = new AvatarLabel();
    m_myAvatar->setNickname(m_myNickname);
    m_myAvatar->setFixedSize(54, 54);
    m_myAvatar->setCursor(Qt::PointingHandCursor);
    m_myAvatar->setToolTip("点击更换头像");
    m_myAvatar->setAvatarColor(m_avatarColor);
    profileBox->addWidget(m_myAvatar, 0, Qt::AlignHCenter);

    auto* profileName = new QLabel(m_myNickname);
    profileName->setStyleSheet(
        "color: #e8eef0; font-size: 14px; font-weight: 800; background: transparent;"
    );
    profileName->setAlignment(Qt::AlignCenter);
    profileName->setFixedWidth(68);
    profileName->setToolTip(m_myNickname);
    profileBox->addWidget(profileName);

    railLayout->addLayout(profileBox);
    railLayout->addSpacing(26);

    // 导航按钮 — 用单字图标避免 emoji 在 macOS 渲染为 3D 立体
    auto* navBox = new QVBoxLayout();
    navBox->setSpacing(10);
    navBox->setAlignment(Qt::AlignHCenter);

    auto makeNavBtn = [this](const QString& text, const QString& tooltip, int idx) -> QPushButton* {
        auto* btn = new QPushButton(text);
        btn->setFixedSize(68, 58);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setToolTip(tooltip);

        btn->setStyleSheet(
            "QPushButton { border: none; border-radius: 8px; background: transparent;"
            "  color: #c7d0d5; font-size: 24px; font-weight: 800; }"
            "QPushButton:hover { background: #262a2e; color: #ffffff; }"
            "QPushButton:checked { background: #262a2e; color: #ffffff; }"
        );

        connect(btn, &QPushButton::clicked, this, [this, idx]() { switchSidebar(idx); });
        return btn;
    };

    auto* msgBtn = makeNavBtn("💬", "聊天", 0);
    navBox->addWidget(msgBtn);
    m_sidebarBtns.append(msgBtn);

    auto* friendBtn = makeNavBtn("👤", "好友", 1);
    navBox->addWidget(friendBtn);
    m_sidebarBtns.append(friendBtn);

    auto* groupBtn = makeNavBtn("👥", "群聊", 2);
    navBox->addWidget(groupBtn);
    m_sidebarBtns.append(groupBtn);

    railLayout->addLayout(navBox);
    railLayout->addStretch();

    // 设置按钮
    auto* settingsBtn = new QPushButton();
    settingsBtn->setFixedSize(68, 58);
    settingsBtn->setCursor(Qt::PointingHandCursor);
    settingsBtn->setToolTip("设置");
    settingsBtn->setStyleSheet(
        "QPushButton { border: 1px solid #515a60; border-radius: 8px;"
        "  background: transparent; color: #c7d0d5; font-size: 24px; font-weight: 800; }"
        "QPushButton:hover { background: #262a2e; color: #ffffff; }"
    );
    settingsBtn->setText("⚙");
    railLayout->addWidget(settingsBtn, 0, Qt::AlignHCenter);

    workspace->addWidget(rail);

    // ══════════════════════════════════════════
    // 中间面板（360px 宽）
    // ══════════════════════════════════════════
    auto* middlePanel = new QWidget();
    middlePanel->setObjectName("middlePanel");
    middlePanel->setFixedWidth(380);
    auto* middleLayout = new QVBoxLayout(middlePanel);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    middleLayout->setSpacing(0);

    // ── 中间面板头部 ──
    auto* sideHead = new QWidget();
    sideHead->setStyleSheet("background: #fbfcfd; border-bottom: 1px solid #dde3e8;");
    auto* sideHeadLayout = new QVBoxLayout(sideHead);
    sideHeadLayout->setContentsMargins(20, 22, 20, 16);
    sideHeadLayout->setSpacing(14);

    // 标题行：好友 (N) + 添加按钮
    auto* titleRow = new QHBoxLayout();
    m_sideTitle = new QLabel("聊天");
    m_sideTitle->setStyleSheet("font-size: 28px; font-weight: 900; color: #202a31; background: transparent;");
    titleRow->addWidget(m_sideTitle);
    m_sideCount = new QLabel();
    m_sideCount->setStyleSheet("color: #6f7f8b; font-size: 20px; font-weight: 800; background: transparent;");
    titleRow->addWidget(m_sideCount);
    titleRow->addStretch();

    m_sideAddBtn = new QPushButton("+ 添加好友");
    m_sideAddBtn->setFixedHeight(38);
    m_sideAddBtn->setCursor(Qt::PointingHandCursor);
    m_sideAddBtn->setStyleSheet(
        "QPushButton { border: 1px solid rgba(39,194,119,0.34); border-radius: 8px;"
        "  color: #158a57; background: #e9f8ef; font-weight: 900; font-size: 15px; padding: 0 14px; }"
        "QPushButton:hover { background: #d5f0e0; }"
    );
    titleRow->addWidget(m_sideAddBtn);
    sideHeadLayout->addLayout(titleRow);

    // 搜索框
    auto* searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("搜索好友、昵称或账号");
    searchEdit->setFixedHeight(40);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dde3e8; border-radius: 8px;"
        "  padding: 0 16px; background: #ffffff; font-size: 15px; color: #202a31;"
        "  selection-background-color: #27c277; }"
        "QLineEdit:focus { border-color: rgba(60,130,230,0.58); }"
    );
    sideHeadLayout->addWidget(searchEdit);

    middleLayout->addWidget(sideHead);

    // ── Segmented tabs ──
    auto* tabRow = new QHBoxLayout();
    tabRow->setContentsMargins(20, 12, 20, 0);
    tabRow->setSpacing(6);

    auto* tabBg = new QWidget();
    tabBg->setStyleSheet(
        "background: #ffffff; border: 1px solid #dde3e8; border-radius: 8px;"
    );
    auto* tabBgLayout = new QHBoxLayout(tabBg);
    tabBgLayout->setContentsMargins(5, 5, 5, 5);
    tabBgLayout->setSpacing(6);

    auto* allBtn = new QPushButton("全部");
    allBtn->setFixedHeight(38);
    allBtn->setCheckable(true);
    allBtn->setChecked(true);
    allBtn->setStyleSheet(
        "QPushButton { border: none; border-radius: 6px; color: #7a8790;"
        "  background: transparent; font-weight: 900; font-size: 15px; }"
        "QPushButton:checked { color: #202a31; background: #edf3f0; }"
    );
    auto* onlineBtn = new QPushButton("在线");
    onlineBtn->setFixedHeight(38);
    onlineBtn->setCheckable(true);
    onlineBtn->setStyleSheet(
        "QPushButton { border: none; border-radius: 6px; color: #7a8790;"
        "  background: transparent; font-weight: 900; font-size: 15px; }"
        "QPushButton:checked { color: #202a31; background: #edf3f0; }"
    );

    tabBgLayout->addWidget(allBtn);
    tabBgLayout->addWidget(onlineBtn);
    tabRow->addWidget(tabBg);
    middleLayout->addLayout(tabRow);

    // ── 列表区域 ──
    m_middleStack = new QStackedWidget();
    m_middleStack->setStyleSheet("QStackedWidget { background: #f7f8fa; }");

    m_convList = new ConversationListWidget(m_socket);
    m_middleStack->addWidget(m_convList);

    // 好友列表包装：上面加 "好友列表" section label
    auto* friendWrapper = new QWidget();
    auto* friendWrapLayout = new QVBoxLayout(friendWrapper);
    friendWrapLayout->setContentsMargins(0, 0, 0, 0);
    friendWrapLayout->setSpacing(0);

    auto* friendsSectionLabel = new QLabel("好友列表");
    friendsSectionLabel->setStyleSheet(
        "font-size: 15px; font-weight: 900; color: #3e4b54; padding: 14px 20px 8px; background: transparent;"
    );
    friendWrapLayout->addWidget(friendsSectionLabel);

    m_friendList = new FriendListWidget(m_socket, m_myUserId);
    friendWrapLayout->addWidget(m_friendList, 1);
    m_middleStack->addWidget(friendWrapper);

    m_groupList = new GroupListWidget(m_socket, m_myUserId);
    m_middleStack->addWidget(m_groupList);

    middleLayout->addWidget(m_middleStack, 1);

    // ── 底部栏 ──
    auto* sideFoot = new QWidget();
    sideFoot->setStyleSheet("background: #ffffff; border-top: 1px solid #dde3e8;");
    auto* footLayout = new QHBoxLayout(sideFoot);
    footLayout->setContentsMargins(20, 12, 20, 12);

    auto* syncLabel = new QLabel("🔄 最近同步：刚刚");
    syncLabel->setStyleSheet("color: #71808a; font-size: 14px; font-weight: 700; background: transparent;");
    footLayout->addWidget(syncLabel);
    footLayout->addStretch();

    auto* verLabel = new QLabel("v0.3");
    verLabel->setStyleSheet("color: #71808a; font-size: 14px; font-weight: 700; background: transparent;");
    footLayout->addWidget(verLabel);

    middleLayout->addWidget(sideFoot);

    workspace->addWidget(middlePanel);

    // ══════════════════════════════════════════
    // 右侧面板（填充剩余）
    // ══════════════════════════════════════════
    auto* rightPanel = new QWidget();
    rightPanel->setObjectName("rightPanel");
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // ── 聊天头部 ──
    auto* chatHead = new QWidget();
    chatHead->setStyleSheet("background: #ffffff; border-bottom: 1px solid #dde3e8;");
    auto* chatHeadLayout = new QHBoxLayout(chatHead);
    chatHeadLayout->setContentsMargins(26, 0, 26, 0);
    chatHead->setMinimumHeight(72);

    // 左侧：聊天图标 + 标题
    auto* chatIconBox = new QHBoxLayout();
    chatIconBox->setSpacing(12);

    auto* chatIcon = new QLabel("💬");
    chatIcon->setFixedSize(42, 42);
    chatIcon->setAlignment(Qt::AlignCenter);
    chatIcon->setStyleSheet(
        "font-size: 22px; background: #e9f8ef; border-radius: 8px; color: #158a57; border: 1px solid #d9efe3;"
    );
    chatIconBox->addWidget(chatIcon);

    auto* chatTitleBox = new QVBoxLayout();
    chatTitleBox->setSpacing(2);
    auto* chatTitle = new QLabel("会话预览");
    chatTitle->setStyleSheet("font-size: 20px; font-weight: 900; color: #202a31; background: transparent;");
    chatTitleBox->addWidget(chatTitle);
    auto* chatSubtitle = new QLabel("选择好友后显示聊天记录、输入框和在线状态");
    chatSubtitle->setStyleSheet("font-size: 14px; color: #6f7f8b; font-weight: 700; background: transparent;");
    chatTitleBox->addWidget(chatSubtitle);
    chatIconBox->addLayout(chatTitleBox);

    chatHeadLayout->addLayout(chatIconBox);
    chatHeadLayout->addStretch();

    // 右侧工具按钮
    auto* searchTool = new QPushButton("🔍");
    searchTool->setFixedSize(38, 38);
    searchTool->setCursor(Qt::PointingHandCursor);
    searchTool->setStyleSheet(
        "QPushButton { border: 1px solid #dde3e8; border-radius: 8px;"
        "  color: #4e5f69; background: #ffffff; font-size: 18px; font-weight: 800; }"
        "QPushButton:hover { color: #3c82e6; border-color: rgba(60,130,230,0.35); }"
    );
    chatHeadLayout->addWidget(searchTool);

    auto* moreTool = new QPushButton("⋯");
    moreTool->setFixedSize(38, 38);
    moreTool->setCursor(Qt::PointingHandCursor);
    moreTool->setStyleSheet(
        "QPushButton { border: 1px solid #dde3e8; border-radius: 8px;"
        "  color: #4e5f69; background: #ffffff; font-size: 20px; font-weight: 900; }"
        "QPushButton:hover { color: #3c82e6; border-color: rgba(60,130,230,0.35); }"
    );
    chatHeadLayout->addWidget(moreTool);

    rightLayout->addWidget(chatHead);

    // ── 聊天内容区（empty state + chat widgets）──
    m_rightStack = new QStackedWidget();
    m_rightStack->setStyleSheet("background: #ffffff;");

    // Empty state 页面
    m_emptyChatWidget = new QWidget();
    auto* emptyLayout = new QVBoxLayout(m_emptyChatWidget);
    emptyLayout->setContentsMargins(34, 34, 34, 34);
    emptyLayout->setAlignment(Qt::AlignCenter);

    // 图标框
    auto* emptyIcon = new QLabel("💬");
    emptyIcon->setFixedSize(132, 112);
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyIcon->setStyleSheet(
        "font-size: 48px; color: #829aa6; border: 1px solid #dde3e8; border-radius: 8px;"
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "  stop:0 rgba(39,194,119,0.12), stop:1 rgba(60,130,230,0.12));"
    );
    emptyLayout->addWidget(emptyIcon, 0, Qt::AlignHCenter);
    emptyLayout->addSpacing(24);

    auto* emptyTitle = new QLabel("选择会话开始聊天");
    emptyTitle->setStyleSheet("font-size: 28px; font-weight: 900; color: #4f5c65; background: transparent;");
    emptyTitle->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyTitle);
    emptyLayout->addSpacing(10);

    auto* emptyDesc = new QLabel("当前还没有选中的好友。添加好友或切换到聊天列表后，就可以查看历史消息。");
    emptyDesc->setStyleSheet("font-size: 16px; color: #6f7f8b; font-weight: 700; background: transparent;");
    emptyDesc->setAlignment(Qt::AlignCenter);
    emptyDesc->setWordWrap(true);
    emptyLayout->addWidget(emptyDesc);
    emptyLayout->addSpacing(24);

    // 快捷操作按钮
    auto* actionRow = new QHBoxLayout();
    actionRow->setAlignment(Qt::AlignCenter);
    actionRow->setSpacing(10);

    auto* addFriendBtn = new QPushButton("+ 添加好友");
    addFriendBtn->setFixedHeight(40);
    addFriendBtn->setCursor(Qt::PointingHandCursor);
    addFriendBtn->setStyleSheet(
        "QPushButton { border: 1px solid #27c277; border-radius: 8px;"
        "  color: #ffffff; background: #27c277; font-weight: 900; font-size: 15px; padding: 0 16px; }"
        "QPushButton:hover { background: #158a57; }"
    );
    actionRow->addWidget(addFriendBtn);

    auto* createGroupBtn = new QPushButton("👥 创建群聊");
    createGroupBtn->setFixedHeight(40);
    createGroupBtn->setCursor(Qt::PointingHandCursor);
    createGroupBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dde3e8; border-radius: 8px;"
        "  color: #46535b; background: #ffffff; font-weight: 900; font-size: 15px; padding: 0 16px; }"
        "QPushButton:hover { background: #f5f6f8; }"
    );
    actionRow->addWidget(createGroupBtn);

    emptyLayout->addLayout(actionRow);
    emptyLayout->addStretch();

    m_rightStack->addWidget(m_emptyChatWidget); // 0

    m_chatWidget = new ChatWidget(m_socket, m_myUserId, m_myNickname);
    m_rightStack->addWidget(m_chatWidget);  // 1

    m_groupChatWidget = new GroupChatWidget(m_socket, m_myUserId, m_myNickname);
    m_rightStack->addWidget(m_groupChatWidget); // 2

    rightLayout->addWidget(m_rightStack, 1);

    // ── 底部输入区（composer）──
    m_composerWidget = new QWidget();
    m_composerWidget->setStyleSheet("background: rgba(255,255,255,0.9); border-top: 1px solid #dde3e8;");
    auto* composerLayout = new QVBoxLayout(m_composerWidget);
    composerLayout->setContentsMargins(0, 0, 0, 0);
    composerLayout->setSpacing(0);

    // 工具栏
    auto* toolBar = new QHBoxLayout();
    toolBar->setContentsMargins(20, 10, 20, 0);
    toolBar->setSpacing(8);

    auto* emojiToolBtn = new QPushButton("😊");
    emojiToolBtn->setFixedSize(34, 34);
    emojiToolBtn->setCursor(Qt::PointingHandCursor);
    emojiToolBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dde3e8; border-radius: 8px;"
        "  background: #ffffff; color: #202a31; font-size: 18px; }"
        "QPushButton:hover { background: #f5f8fa; }"
    );
    toolBar->addWidget(emojiToolBtn);

    auto* imageToolBtn = new QPushButton("🖼");
    imageToolBtn->setFixedSize(34, 34);
    imageToolBtn->setCursor(Qt::PointingHandCursor);
    imageToolBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dde3e8; border-radius: 8px;"
        "  background: #ffffff; color: #202a31; font-size: 16px; }"
        "QPushButton:hover { background: #f5f8fa; }"
    );
    toolBar->addWidget(imageToolBtn);
    toolBar->addStretch();

    composerLayout->addLayout(toolBar);

    // 输入行
    auto* inputRow = new QHBoxLayout();
    inputRow->setContentsMargins(20, 10, 20, 18);
    inputRow->setSpacing(12);

    auto* inputEdit = new QLineEdit();
    inputEdit->setPlaceholderText("选择好友后即可输入消息");
    inputEdit->setReadOnly(true);
    inputEdit->setFixedHeight(44);
    inputEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dde3e8; border-radius: 8px;"
        "  padding: 0 16px; font-size: 16px; color: #202a31;"
        "  background: #ffffff; selection-background-color: #27c277; }"
        "QLineEdit:read-only { color: #6f7f8b; background: #ffffff; }"
    );
    inputRow->addWidget(inputEdit, 1);

    auto* sendBtn = new QPushButton("发送 ▸");
    sendBtn->setFixedSize(80, 44);
    sendBtn->setEnabled(false);
    sendBtn->setCursor(Qt::PointingHandCursor);
    sendBtn->setStyleSheet(
        "QPushButton { border: none; border-radius: 8px;"
        "  color: #ffffff; background: #27c277; font-weight: 900; font-size: 15px; }"
        "QPushButton:disabled { color: #ffffff; background: #b7c9bf; }"
    );
    inputRow->addWidget(sendBtn);

    composerLayout->addLayout(inputRow);

    rightLayout->addWidget(m_composerWidget);

    workspace->addWidget(rightPanel, 1);

    mainVBox->addLayout(workspace, 1);
    rootLayout->addLayout(mainVBox);

    // ══════════════════════════════════════════
    // 信号连接
    // ══════════════════════════════════════════
    connect(m_convList, &ConversationListWidget::conversationClicked,
            this, &MainHubWidget::onOpenChat);
    connect(m_friendList, &FriendListWidget::startChat,
            this, &MainHubWidget::onOpenChat);
    connect(m_groupList, &GroupListWidget::openGroupChat,
            this, &MainHubWidget::onOpenGroupChat);

    connect(m_myAvatar, &AvatarLabel::clicked, this, &MainHubWidget::onChangeAvatar);
    connect(m_friendList, &FriendListWidget::friendsLoaded,
            this, &MainHubWidget::onFriendsLoaded);

    connect(addFriendBtn, &QPushButton::clicked, this, [this]() {
        switchSidebar(1);
    });
    connect(createGroupBtn, &QPushButton::clicked, this, [this]() {
        switchSidebar(2);
    });
    connect(m_sideAddBtn, &QPushButton::clicked, this, [this]() {
        // 根据当前选中的侧栏执行不同操作
        int idx = -1;
        for (int i = 0; i < m_sidebarBtns.size(); ++i) {
            if (m_sidebarBtns[i]->isChecked()) { idx = i; break; }
        }
        if (idx == 1) {
            // 好友 tab: 打开添加好友对话框
            AddFriendDialog dlg(m_socket, this);
            if (dlg.exec() == QDialog::Accepted) {
                m_friendList->refreshFriends();
                m_friendList->refreshPending();
            }
        } else if (idx == 2) {
            // 群聊 tab: 打开创建群聊对话框
            // 触发 GroupCreateDialog
            auto* dlg = new GroupCreateDialog(m_socket, this);
            if (dlg->exec() == QDialog::Accepted) {
                // 刷新群列表
                m_groupList->refreshGroups();
            }
            delete dlg;
        }
    });

    // 默认选中"聊天"
    switchSidebar(0);

    // 登录后自动拉取好友列表
    m_friendList->refreshFriends();
}

void MainHubWidget::switchSidebar(int index)
{
    if (index < 0 || index >= m_sidebarBtns.size()) return;
    setActiveSidebar(index);
    m_middleStack->setCurrentIndex(index);

    // 更新中间面板标题
    switch (index) {
    case 0:
        m_sideTitle->setText("聊天");
        m_sideCount->clear();
        m_sideAddBtn->hide();
        break;
    case 1:
        m_sideTitle->setText("好友");
        m_sideAddBtn->setText("+ 添加好友");
        m_sideAddBtn->show();
        m_friendList->refreshFriends();
        m_friendList->refreshPending();
        break;
    case 2:
        m_sideTitle->setText("群聊");
        m_sideAddBtn->setText("+ 创建群聊");
        m_sideAddBtn->show();
        m_groupList->refreshGroups();
        break;
    }
}

void MainHubWidget::setActiveSidebar(int index)
{
    for (int i = 0; i < m_sidebarBtns.size(); ++i) {
        m_sidebarBtns[i]->setChecked(i == index);
    }
}

void MainHubWidget::onOpenChat(int userId, const QString& nickname)
{
    m_chatWidget->openConversation(userId, nickname);
    m_rightStack->setCurrentIndex(1);
    m_composerWidget->hide(); // ChatWidget 自带输入区
}

void MainHubWidget::onOpenGroupChat(int groupId, const QString& name)
{
    m_groupChatWidget->openGroup(groupId, name);
    m_rightStack->setCurrentIndex(2);
    m_composerWidget->hide(); // GroupChatWidget 自带输入区
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
    // 更新好友计数
    m_sideCount->setText(QString("(%1)").arg(friends.size()));

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
