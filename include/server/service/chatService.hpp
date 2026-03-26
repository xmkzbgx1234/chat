#ifndef CHAT_SERVICE_HPP
#define CHAT_SERVICE_HPP

#include <unordered_map>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include "baseService.hpp"
#include "UserModel.hpp"
#include "offLineMsgModel.hpp"
#include "groupModel.hpp"
#include "chatMessageModel.hpp"
#include "redis.hpp"
#include "onlineUserManager.hpp"

class ChatService : public BaseService{
private:
    UserModel& _userModel;
    OffLineMsgModel& _offLineMsgModel;
    GroupModel& _groupModel;
    ChatMessageModel& _chatMessageModel;
    Redis& _redis;
    OnlineUserManager& _onlineUserManager;

public:
    // 构造函数
    ChatService(UserModel& userModel, OffLineMsgModel& offLineMsgModel,
                GroupModel& groupModel, ChatMessageModel& chatMessageModel,
                Redis& redis, OnlineUserManager& onlineUserManager);
    ~ChatService() = default;
    // 处理消息
    void handleMessage(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time) override;
    void oneChat(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
    void groupChat(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
};

#endif
