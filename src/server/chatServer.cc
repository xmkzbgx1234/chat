#include <string>
#include <functional>
#include <cstring>
#include <nlohmann/json.hpp>
#include "chatServer.hpp"  
#include "IService.hpp"
#include "log.h"
#include "public.hpp"
#include "responseBuilder.hpp"
#include "errorCode.hpp"

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

    _server.setThreadNum(6); // 1 个 main loop 线程 6 个 sub loop(I/O) 线程
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
    while (buffer->readableBytes() >= header_size)
    {
        uint32_t networkSize = 0;
        std::memcpy(&networkSize, buffer->peek(), header_size);
        const uint32_t bodySize = ntohl(networkSize);
        if (bodySize > max_body_size)
        {
            LOG_WARN << "Invalid packet size=" << bodySize << " from " << conn->peerAddress().toIpPort();
            conn->shutdown();
            return;
        }
        if (buffer->readableBytes() < header_size + bodySize)
        {
            break;
        }

        buffer->retrieve(header_size);
        std::string msg(buffer->peek(), bodySize);
        buffer->retrieve(bodySize);
        LOG_DEBUG << "recv data:" << msg << " time:" << receiveTime.toString();

        json js = json::parse(msg, nullptr, false);
        if (js.is_discarded())
        {
            LOG_WARN << "Parse client JSON failed from " << conn->peerAddress().toIpPort();
            conn->send(encodeMessage(ResponseBuilder::error(-1, ErrorCode::INVALID_PARAM, "JSON解析失败").dump()));
            continue;
        }

        if (!js.contains("msgid") || !js["msgid"].is_number_integer())
        {
            LOG_WARN << "Invalid msgid from " << conn->peerAddress().toIpPort();
            conn->send(encodeMessage(ResponseBuilder::error(-1, ErrorCode::INVALID_PARAM, "无效的消息ID").dump()));
            continue;
        }

        auto msgHandler = IService::instance()->getHandler(js["msgid"].get<int>());
        if (msgHandler)
        {
            msgHandler(conn, js, receiveTime);
        }
    }
}
