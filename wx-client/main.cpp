// TinyWeChat Client — Phase 1 MVP
// 功能：登录 + 单聊文本收发 + 历史拉取
//
// 运行方式（需要先启动服务端）：
//   客户端1: ./tinywechat-client  (登录 alice)
//   客户端2: ./tinywechat-client  (登录 bob)

#include <QApplication>
#include <iostream>
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("TinyWeChat Client");

    std::cout << "[client] TinyWeChat Client v0.2 — Phase 1 MVP" << std::endl;
    std::cout << "[client] 默认连接 127.0.0.1:9090" << std::endl;

    MainWindow window;
    window.show();

    return app.exec();
}
