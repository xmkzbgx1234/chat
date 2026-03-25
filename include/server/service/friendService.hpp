#ifndef FRIEND_SERVICE_HPP
#define FRIEND_SERVICE_HPP
#include "userModel.hpp"
#include "friendModel.hpp"
#include "redis.hpp"
#include "baseService.hpp"
#include <mutex>

class FriendService : public BaseService
{
private:
public:
    FriendService(UserModel* userModel, FriendModel* friendModel);
    ~FriendService() = default;
    // 处理消息
    void handleMessage(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time) override;

    void addFriend(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
};


#endif