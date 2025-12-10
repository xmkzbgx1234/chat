/* 基于muduo网络库开发服务器程序
1. 组合TcpServer对象
2. 创建EventLoop事件循环对象
3. 明确TcpServer构造函数需要的参数
4. 在服务器类的构造函数中注册连接回调和消息回调
5. 设置合适的线程数量，muduo库会自动分配I/O线程和worker线程
6. 调用TcpServer对象的start()方法，开启服务器监听
7. 调用EventLoop对象的loop()方法，启动事件循环
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <string>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

class ChatServer
{
public:
    ChatServer(EventLoop *loop, // 事件循环
               const InetAddress &listenAddr, // IP + Port
               const string &name) // 服务器名称
        : _server(loop, listenAddr, name),
          _loop(loop)
    {   
        // 注册连接回调和消息回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this, _1));
        _server.setMessageCallback(std::bind(&ChatServer::onMessage,this, _1, _2, _3));

        // muduo库自动分配一个线程处理网络事件 五个线程处理用户的业务逻辑
        _server.setThreadNum(6); 
    }

    // 开启事件循环
    void start()
    {
        _server.start();
    }
private:
    // 处理新连接和断开连接事件
    void onConnection(const TcpConnectionPtr &conn)
    {
        if(conn->connected()){
            cout << conn->peerAddress().toIpPort() << " -> "
                 << conn->localAddress().toIpPort() << " state:online" << endl;
        }
        else{
            cout << conn->peerAddress().toIpPort() << " -> "
                 << conn->localAddress().toIpPort() << " state:offline" << endl;
            conn->shutdown();
        }
    }
    // 处理用户读写事件
    void onMessage(const TcpConnectionPtr &conn, // 连接
                   Buffer *buf, // 缓冲区
                   Timestamp time) // 接收数据的时间点
    {
        string msg = buf->retrieveAllAsString();
        cout << "recv data:" << msg << " time:" << time.toString() << endl;

        // 回显数据
        conn->send(msg);
    }
    TcpServer _server;
    EventLoop *_loop;
};

int main()
{
    EventLoop loop; // 创建事件循环对象
    InetAddress addr("127.0.0.1", 8888); // 服务器IP和端口
    ChatServer server(&loop, addr, "ChatServer");
    server.start(); // listenfd 通过epoll_ctl注册到epoll树上
    loop.loop(); // 启动事件循环，以阻塞方式等待新用户连接，已连接用户的读写事件等（epoll_wait）
    return 0;
}