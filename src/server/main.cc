#include "chatServer.hpp"
#include "IService.hpp"
#include <iostream>
#include <signal.h>
#include <string>

using namespace std;

// 处理服务器异常退出
void resetHandler(int sig)
{
    IService::instance()->reset();
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, resetHandler); // 注册信号处理函数
    EventLoop loop; // 创建事件循环对象
    if(argc != 3){
        cout << "Usage: " << argv[0] << " <ip> <port>" << endl;
        return -1;
    }
    string ip = argv[1];
    int port = atoi(argv[2]);

    InetAddress addr(ip, port); // 服务器IP和端口
    IService::instance()->reset();
    ChatServer server(&loop, addr, "ChatServer"); // 创建服务器对象

    server.start(); // 启动服务器

    loop.loop(); // 启动事件循环

    return 0;
} 