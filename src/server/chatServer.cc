#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include "chatServer.hpp"  
#include "IService.hpp"
#include "log.h"

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, 
                        const string& nameAgs)
                        :_server(loop, listenAddr, nameAgs),
                         _loop(loop)
{
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    _server.setThreadNum(6); // 1个I/O线程 5个worker线程
}

void ChatServer::start(){
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr & conn){
    if(conn->connected()){
        LOG_INFO << conn->peerAddress().toIpPort() << " -> "
                 << conn->localAddress().toIpPort() << " state:online";
    }
    else{
        LOG_INFO << conn->peerAddress().toIpPort() << " -> "
                 << conn->localAddress().toIpPort() << " state:offline";
        // 处理客户端异常退出
        IService::instance()->ClientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr & conn, 
    Buffer * buffer, Timestamp receiveTime){
    std::string msg = buffer->retrieveAllAsString();
    LOG_DEBUG << "recv data:" << msg << " time:" << receiveTime.toString();
    // 数据json反序列化
    json js = json::parse(msg);
    // 通过json["msgid"] 判断消息类型，进行不同的server handler
    // 完全解耦网络模块和业务模块
    auto msgHandler = IService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息处理方法
    if(msgHandler){
        msgHandler(conn, js, receiveTime);
    }
}
