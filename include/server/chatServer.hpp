#ifndef CHATSERVER_HPP
#define CHATSERVER_HPP

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    // 初始化聊天服务器
    ChatServer(EventLoop * loop, 
                const InetAddress & listenAddr,
                const string & nameArg);
    // 启动服务器
    void start();

private:
    TcpServer _server; // 组合TcpServer对象
    EventLoop *_loop;  // 指向事件循环对象的指针
    void onConnection(const TcpConnectionPtr & conn); // 连接回调函数
    void onMessage(const TcpConnectionPtr & conn, 
        Buffer * buffer, Timestamp receiveTime); // 消息回调函数

};

#endif // CHATSERVER_H