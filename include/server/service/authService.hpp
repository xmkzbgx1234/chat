#ifndef AUTH_SERVICE_HPP
#define AUTH_SERVICE_HPP

#include <muduo/net/TcpConnection.h>
#include <nlohmann/json.hpp> 
#include <mutex>
#include "baseService.hpp"
#include "UserModel.hpp"
#include "offLineMsgModel.hpp"
#include "friendModel.hpp"
#include "groupModel.hpp"
#include "chatMessageModel.hpp"
#include "redis.hpp"
#include "onlineUserManager.hpp"

class AuthService : public BaseService
{
private:
    UserModel& _userModel;
    OffLineMsgModel& _offLineMsgModel;
    FriendModel& _friendModel;
    GroupModel& _groupModel;
    ChatMessageModel& _chatMessageModel;
    Redis& _redis;
    OnlineUserManager& _onlineUserManager;

public:
    AuthService(UserModel& userModel, OffLineMsgModel& offLineMsgModel,
                FriendModel& friendModel, GroupModel& groupModel,
                ChatMessageModel& chatMessageModel,
                Redis& redis, OnlineUserManager& onlineUserManager);
    ~AuthService() = default;
    // 处理消息
    void handleMessage(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time) override;

    void login(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time);
    void regist(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time);
    void loginout(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time);
};

#endif
