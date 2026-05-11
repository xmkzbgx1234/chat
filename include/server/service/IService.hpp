#ifndef I_SERVICE_HPP
#define I_SERVICE_HPP

#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>
#include <muduo/net/TcpConnection.h>
#include <nlohmann/json.hpp>
#include "UserModel.hpp"
#include "offLineMsgModel.hpp"
#include "friendModel.hpp"
#include "groupModel.hpp"
#include "chatMessageModel.hpp"
#include "redis.hpp"
#include "chatService.hpp"
#include "groupService.hpp"
#include "friendService.hpp"
#include "authService.hpp"
#include "onlineUserManager.hpp"

using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time)>;

// 服务接口类
class IService{
public:
    static IService* instance();
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    // 处理客户端异常退出
    void ClientCloseException(const muduo::net::TcpConnectionPtr& conn);
    void reset(); // 重置服务状态
    void handleRedisMessage(int channel, const std::string &message);

private:
    IService();
    // 存储消息id和对应的业务处理方法 
    std::unordered_map<int, MsgHandler> _msgHandlerMap;

    // 数据操作类对象
    UserModel _userModel;

    // 离线消息操作类对象
    OffLineMsgModel _offLineMsgModel;

    // 好友操作类对象
    FriendModel _friendModel;

    // 群组操作类对象
    GroupModel _groupModel;

    // 聊天记录操作类对象
    ChatMessageModel _chatMessageModel;

    // Redis 操作类对象
    Redis _redis;

    // 在线用户管理类对象
    OnlineUserManager _onlineUserManager;

    ChatService _chatService;
    GroupService _groupService;
    FriendService _friendService;
    AuthService _authService;
};


#endif
