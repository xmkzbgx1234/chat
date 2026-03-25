#ifndef AUTH_SERVICE_HPP
#define AUTH_SERVICE_HPP

#include <muduo/net/TcpConnection.h>
#include <nlohmann/json.hpp> 
#include <mutex>
#include "baseService.hpp"
#include "userModel.hpp"
#include "offLineMsgModel.hpp"
#include "friendModel.hpp"
#include "groupModel.hpp"
#include "redis.hpp"
#include "onlineUserManager.hpp"

class AuthService : public BaseService
{
public:
    AuthService(UserModel* userModel, OffLineMsgModel* offLineMsgModel,
                             FriendModel* friendModel, GroupModel* groupModel,
                             Redis* redis, OnlineUserManager* onlineUserManager);
    ~AuthService() = default;
    // 处理消息
    void handleMessage(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time) override;

    void login(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time);
    void regist(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time);
    void loginout(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time);
};

#endif