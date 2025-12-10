#ifndef CHAT_SERVICE_HPP
#define CHAT_SERVICE_HPP

#include <unordered_map>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include "UserModel.hpp"
#include "offLineMsgModel.hpp"
#include "friendModel.hpp"
#include "groupModel.hpp"
#include "redis.hpp"

using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time)>;

// 聊天服务器业务类 单例模式
class ChatService{
public:
    // 获取单例对象的接口函数
    static ChatService* instance();
    void login(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
    void regist(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
    void oneChat(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
    void addFriend(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
    void createGroup(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
    void addGroup(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
    void groupChat(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
    void loginout(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
    void reset(); // 重置服务状态
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    // 处理Redis消息
    void handleRedisMessage(int channel, const std::string &message);

    void ClientCloseException(const muduo::net::TcpConnectionPtr& conn);
private:
    ChatService();
    // 存储消息id和对应的业务处理方法 
    std::unordered_map<int, MsgHandler> _msgHandlerMap;

    // 数据操作类对象
    UserModel _userModel;

    // 离线消息操作类对象
    OffLineMessageModel _offLineMsgModel;

    // 好友操作类对象
    FriendModel _friendModel;

    // 群组操作类对象
    GroupModel _groupModel;

    // 定义互斥锁
    std::mutex _connMutex;
    // 存储在线用户的通信连接
    std::unordered_map<int, muduo::net::TcpConnectionPtr> _userConnMap;

    // Redis 操作类对象
    Redis _redis;
};

#endif