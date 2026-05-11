#include "IService.hpp"
#include <unordered_map>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include "public.hpp"
#include "UserModel.hpp"
#include "offLineMsgModel.hpp"
#include "friendModel.hpp"
#include "groupModel.hpp"
#include "redis.hpp"
#include "chatService.hpp"
#include "groupService.hpp"
#include "friendService.hpp"
#include "authService.hpp"
#include "onlineUserManager.hpp"
#include "log.h"

#define MSGHANDLER(msg, handler) _msgHandlerMap[msg] = [this](const TcpConnectionPtr& conn, json& js, Timestamp time) { handler(conn, js, time);}

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

IService* IService::instance()
{
    static IService instance;
    return &instance;
}

IService::IService() : 
    _userModel(UserModel()),
    _offLineMsgModel(OffLineMsgModel()),
    _friendModel(FriendModel()),
    _groupModel(GroupModel()),
    _chatMessageModel(ChatMessageModel()),
    _redis(Redis()),
    _onlineUserManager(),
    _chatService(_userModel, _offLineMsgModel, _groupModel, _chatMessageModel, _redis, _onlineUserManager),
    _groupService(_groupModel),
    _friendService(_userModel, _friendModel),
    _authService(_userModel, _offLineMsgModel, _friendModel, _groupModel, _chatMessageModel, _redis, _onlineUserManager)
{

    // 1. 注册消息处理函数
    MSGHANDLER(LOGIN_MSG, _authService.handleMessage);
    MSGHANDLER(REG_MSG, _authService.handleMessage);
    MSGHANDLER(LOGINOUT_MSG, _authService.handleMessage);
    MSGHANDLER(ADD_FRIEND_MSG, _friendService.handleMessage);
    MSGHANDLER(ADD_GROUP_MSG, _groupService.handleMessage);
    MSGHANDLER(CREATE_GROUP_MSG, _groupService.handleMessage);
    MSGHANDLER(ONE_CHAT_MSG, _chatService.handleMessage);
    MSGHANDLER(GROUP_CHAT_MSG, _chatService.handleMessage);

    // 2. 连接Redis
    if (_redis.connect()) {
        _redis.initNotifyMessageHandler([this](int channel, const std::string& message) {
            handleRedisMessage(channel, message);
        });
    }
}

MsgHandler IService::getHandler(int msgid)
{
    if(_msgHandlerMap.find(msgid) == _msgHandlerMap.end())
    {
        return nullptr;
    }
    return _msgHandlerMap[msgid];
}

void IService::reset()  
{
    // 把在线用户的状态设置为离线
    _userModel.resetState();
}

void IService::ClientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    // 遍历_userConnMap，找到对应的连接并删除
    int channel = _onlineUserManager.removeByConn(conn);
    if(channel == -1)
    {
        return; // 用户不存在，直接返回
    }
    _redis.unsubscribe(channel);
    user = _userModel.getUserById(channel);
    if (user.getId() == -1)
    {
        return; // 用户不存在，直接返回
    }
    user.setState("offline");
    _userModel.updateUserInfo(user);
}

void IService::handleRedisMessage(int channel, const string &message){

    TcpConnectionPtr conn = _onlineUserManager.getUserConn(channel);
    if(conn != nullptr)
    {
        LOG_DEBUG << "handleRedisMessage: channel=" << channel << ", message=" << message;
        conn->send(encodeMessage(message));
        return;
    }
    // 存储该用户的离线消息
    _offLineMsgModel.insert(channel, message);
} 
