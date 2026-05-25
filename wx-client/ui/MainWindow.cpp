#include "MainWindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_socket(new WeChatSocket(this))
    , m_stack(new QStackedWidget(this))
{
    setWindowTitle("TinyWeChat");
    resize(500, 580);

    // 窗口默认背景
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor("#f4f7f6"));
    m_stack->setAutoFillBackground(true);
    m_stack->setPalette(pal);

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
    }
    m_stack->setCurrentIndex(Page::HUB);
    setWindowTitle(QString("TinyWeChat — %1").arg(nickname));
    resize(1100, 720);
}

void MainWindow::onGoRegister()
{
    m_stack->setCurrentIndex(Page::REGISTER);
    resize(500, 620);
}

void MainWindow::onRegistered(const QString&)
{
    m_stack->setCurrentIndex(Page::LOGIN);
    resize(500, 580);
}

void MainWindow::onBackToLogin()
{
    m_stack->setCurrentIndex(Page::LOGIN);
    resize(500, 580);
}
