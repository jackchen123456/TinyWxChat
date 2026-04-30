#include "MainWindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_socket(new WeChatSocket(this))
    , m_stack(new QStackedWidget(this))
{
    setWindowTitle("TinyWeChat");
    resize(420, 640);

    // 登录页面
    m_loginWidget = new LoginWidget(m_socket);
    m_stack->addWidget(m_loginWidget);   // index 0

    setCentralWidget(m_stack);

    connect(m_loginWidget, &LoginWidget::loggedIn, this, &MainWindow::onLoggedIn);
}

void MainWindow::onLoggedIn(int userId, const QString& nickname)
{
    qDebug() << "[MainWindow] User logged in:" << userId << nickname;

    // 创建聊天页面（共用同一个 m_socket）
    m_chatWidget = new ChatWidget(m_socket, userId, nickname);
    m_stack->addWidget(m_chatWidget);     // index 1

    // 切换到聊天页面
    m_stack->setCurrentIndex(1);

    // 窗口标题加上用户昵称
    setWindowTitle(QString("TinyWeChat — %1").arg(nickname));
}
