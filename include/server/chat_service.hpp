#ifndef CHAT_SERVICE_HPP
#define CHAT_SERVICE_HPP

#include <unordered_map>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include <nlohmann/json.hpp>
#include "UserModel.hpp"

using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time)>;

// 聊天服务器业务类 单例模式
class ChatService{
public:
    // 获取单例对象的接口函数
    static ChatService* instance();
    void login(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
    void regist(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
private:
    ChatService();
    // 存储消息id和对应的业务处理方法 
    std::unordered_map<int, MsgHandler> _msgHandlerMap;

    // 数据操作类对象
    UserModel _userModel;
};

#endif