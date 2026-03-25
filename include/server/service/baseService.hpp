#ifndef BASE_SERVICE_HPP
#define BASE_SERVICE_HPP
#include <nlohmann/json.hpp>
#include <muduo/net/TcpConnection.h>
#include "onlineUserManager.hpp"
#include "userModel.hpp"
#include "offLineMsgModel.hpp"
#include "friendModel.hpp"
#include "groupModel.hpp"
#include "redis.hpp"


class BaseService{
public:
    virtual ~BaseService() = default;
    virtual void handleMessage(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time) = 0;
protected:
    protected:
    // 子类共享的依赖（避免每个Service都重复定义）
    UserModel* _userModel = nullptr;
    OffLineMsgModel* _offLineMsgModel = nullptr;
    FriendModel* _friendModel = nullptr;
    GroupModel* _groupModel = nullptr;
    Redis* _redis = nullptr;
    OnlineUserManager* _onlineUserManager = nullptr;
};

#endif