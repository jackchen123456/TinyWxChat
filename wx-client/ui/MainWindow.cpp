#include "MainWindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_socket(new WeChatSocket(this))
    , m_stack(new QStackedWidget(this))
{
    setWindowTitle("TinyWeChat");
    resize(420, 680);

    m_loginWidget = new LoginWidget(m_socket);
    m_stack->addWidget(m_loginWidget);  // 0

    m_registerWidget = new RegisterWidget(m_socket);
    m_stack->addWidget(m_registerWidget);  // 1

    setCentralWidget(m_stack);

    connect(m_loginWidget, &LoginWidget::loggedIn,   this, &MainWindow::onLoggedIn);
    connect(m_loginWidget, &LoginWidget::goRegister, this, &MainWindow::onGoRegister);
    connect(m_registerWidget, &RegisterWidget::registered,  this, &MainWindow::onRegistered);
    connect(m_registerWidget, &RegisterWidget::backToLogin, this, &MainWindow::onBackToLogin);
}

void MainWindow::onLoggedIn(int userId, const QString& nickname)
{
    m_myUserId   = userId;
    m_myNickname = nickname;

    if (!m_hubWidget) {
        m_hubWidget = new MainHubWidget(m_socket, userId, nickname);
        m_stack->addWidget(m_hubWidget);  // 2
        connect(m_hubWidget, &MainHubWidget::openChat,      this, &MainWindow::onOpenChat);
        connect(m_hubWidget, &MainHubWidget::openGroupChat, this, &MainWindow::onOpenGroupChat);
    }
    m_stack->setCurrentIndex(Page::HUB);
    setWindowTitle(QString("TinyWeChat — %1").arg(nickname));
}

void MainWindow::onGoRegister()       { m_stack->setCurrentIndex(Page::REGISTER); }
void MainWindow::onRegistered(const QString&) { m_stack->setCurrentIndex(Page::LOGIN); }
void MainWindow::onBackToLogin()      { m_stack->setCurrentIndex(Page::LOGIN); }

void MainWindow::onOpenChat(int userId, const QString& nickname)
{
    if (!m_chatWidget) {
        m_chatWidget = new ChatWidget(m_socket, m_myUserId, m_myNickname);
        m_stack->addWidget(m_chatWidget);  // 3
        connect(m_chatWidget, &ChatWidget::back, this, &MainWindow::onBackToHub);
    }
    m_chatWidget->openConversation(userId, nickname);
    m_stack->setCurrentIndex(Page::CHAT);
}

void MainWindow::onOpenGroupChat(int groupId, const QString& name)
{
    if (!m_groupChatWidget) {
        m_groupChatWidget = new GroupChatWidget(m_socket, m_myUserId, m_myNickname);
        m_stack->addWidget(m_groupChatWidget);  // 4
        connect(m_groupChatWidget, &GroupChatWidget::back, this, &MainWindow::onBackToHub);
    }
    m_groupChatWidget->openGroup(groupId, name);
    m_stack->setCurrentIndex(Page::GROUP_CHAT);
}

void MainWindow::onBackToHub()
{
    m_stack->setCurrentIndex(Page::HUB);
}
