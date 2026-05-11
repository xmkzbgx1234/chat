#include "authService.hpp"
#include "UserModel.hpp"
#include "offLineMsgModel.hpp"
#include "friendModel.hpp"
#include "groupModel.hpp"
#include "redis.hpp"
#include "public.hpp"
#include "log.h"
#include "validator.hpp"
#include "errorCode.hpp"
#include "responseBuilder.hpp"
#include "passwordEncryptor.hpp"
#include <muduo/net/EventLoop.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <unordered_map>

using namespace std;
using json = nlohmann::json;
using namespace muduo::net;
using namespace muduo;

AuthService::AuthService(UserModel &userModel, OffLineMsgModel &offLineMsgModel,
                         FriendModel &friendModel, GroupModel &groupModel,
                         ChatMessageModel &chatMessageModel,
                         Redis &redis, OnlineUserManager &onlineUserManager)
    : _userModel(userModel),
      _offLineMsgModel(offLineMsgModel),
      _friendModel(friendModel),
      _groupModel(groupModel),
      _chatMessageModel(chatMessageModel),
      _redis(redis),
      _onlineUserManager(onlineUserManager)
{}

// 处理消息
void AuthService::handleMessage(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int msgid = js["msgid"].get<int>();
    switch (msgid)
    {
    case LOGIN_MSG:
        login(conn, js, time);
        break;
    case REG_MSG:
        regist(conn, js, time);
        break;
    case LOGINOUT_MSG:
        loginout(conn, js, time);
        break;
    default:
        break;
    }
}



void AuthService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "Handle login request";
    
    // 参数校验
    int id = Validator::getInt(js, "id", -1);
    string password = Validator::getString(js, "password", "");
    
    if (!Validator::isValidUserId(id)) {
        conn->send(encodeMessage(ResponseBuilder::error(LOGIN_MSG_ACK, ErrorCode::INVALID_USER_ID).dump()));
        return;
    }
    
    if (!Validator::isNotEmpty(password)) {
        conn->send(encodeMessage(ResponseBuilder::error(LOGIN_MSG_ACK, ErrorCode::INVALID_PASSWORD, "密码不能为空").dump()));
        return;
    }
    
    User user = _userModel.getUserById(id);

    bool passwordValid = PasswordEncryptor::getInstance().verifyPassword(
        password,           // 用户输入的明文密码
        user.getPassword()  // 数据库中的哈希值
    );

    if (user.getId() == id && passwordValid)
    {
        // 检查是否已登录（踢下线机制）
        TcpConnectionPtr oldConn = _onlineUserManager.getUserConn(id);
        if (oldConn) {
            // 发送踢下线通知
            json kickMsg;
            kickMsg["msgid"] = KICK_OFF_MSG;
            kickMsg["reason"] = "您的账号在其他设备登录，您已被强制下线";
            oldConn->send(encodeMessage(kickMsg.dump()));
            
            // 延迟断开旧连接（给客户端时间接收消息）
            oldConn->getLoop()->runAfter(0.5, [oldConn](){
                if (oldConn->connected()) {
                    oldConn->forceClose();
                }
            });
            
            LOG_INFO << "User " << id << " kicked off old connection, new login from another device";
        }
        
        // 登录成功
        // 更新用户状态信息
        user.setState("online");
        _userModel.updateUserInfo(user);
        // 添加用户连接到在线用户管理器
        _onlineUserManager.addUser(id, conn);
        // 订阅用户的channel
        _redis.subscribe(id);
        
        json response = ResponseBuilder::success(LOGIN_MSG_ACK);
        response["id"] = user.getId();
        response["name"] = user.getName();
        response["state"] = user.getState();
        vector<string> offlinemsg = _offLineMsgModel.get(id);
        if (!offlinemsg.empty())
        {
            // 有离线消息，发送给用户
            response["offlinemsg"] = offlinemsg;
            _offLineMsgModel.remove(id); // 删除离线消息
        }
        vector<User> friends = _friendModel.query(id);
        if (!friends.empty())
        {
            // 好友列表
            vector<json> friends_js;
            for (User &friend_ : friends)
            {
                json js;
                js["id"] = friend_.getId();
                js["name"] = friend_.getName();
                js["state"] = friend_.getState();
                friends_js.push_back(js);
            }
            response["friends"] = friends_js;
        }
        // 群组列表
        vector<Group> groups = _groupModel.queryGroups(id);
        if (!groups.empty())
        {
            vector<json> groups_js;
            for (Group &group : groups)
            {
                json group_js;
                group_js["id"] = group.getId();
                group_js["name"] = group.getName();
                group_js["desc"] = group.getDesc();

                vector<groupUser> users = group.getUsers();
                vector<json> users_js;
                for (groupUser &user : users)
                {
                    json user_js;
                    user_js["id"] = user.getId();
                    user_js["name"] = user.getName();
                    user_js["role"] = user.getRole();
                    user_js["state"] = user.getState();
                    users_js.push_back(user_js);
                }
                group_js["users"] = users_js;
                groups_js.push_back(group_js);
            }
            response["groups"] = groups_js;
        }
        long long syncCursor = 0;
        if (js.contains("sync_cursor"))
        {
            syncCursor = js["sync_cursor"].get<long long>();
        }
        vector<json> historyMessages = _chatMessageModel.queryUserMessages(id, syncCursor);
        if (!historyMessages.empty())
        {
            response["history_messages"] = historyMessages;
            syncCursor = historyMessages.back().value("messageid", syncCursor);
        }
        response["sync_cursor"] = syncCursor;
        conn->send(encodeMessage(response.dump()));
    }
    else
    {
        conn->send(encodeMessage(ResponseBuilder::error(LOGIN_MSG_ACK, ErrorCode::AUTH_LOGIN_FAILED, "用户名或密码错误").dump()));
    }
}

void AuthService::regist(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "Handle register request";
    
    // 参数校验
    string name = Validator::getString(js, "name", "");
    string password = Validator::getString(js, "password", "");
    
    if (!Validator::isValidUsername(name)) {
        conn->send(encodeMessage(ResponseBuilder::error(REG_MSG_ACK, ErrorCode::INVALID_USERNAME, "用户名长度必须在2-64个字符之间").dump()));
        return;
    }
    
    if (!Validator::isValidPassword(password)) {
        conn->send(encodeMessage(ResponseBuilder::error(REG_MSG_ACK, ErrorCode::INVALID_PASSWORD, "密码长度必须在6-128个字符之间").dump()));
        return;
    }
    
    User user = User();
    user.setName(name);
    string hashedPassword = PasswordEncryptor::getInstance().hashPassword(password);
    user.setPassword(hashedPassword);
    bool status = _userModel.insert(user);
    
    if (status)
    {
        json response = ResponseBuilder::success(REG_MSG_ACK);
        response["id"] = user.getId();
        conn->send(encodeMessage(response.dump()));
    }
    else
    {
        conn->send(encodeMessage(ResponseBuilder::error(REG_MSG_ACK, ErrorCode::AUTH_REGISTER_FAILED, "注册失败，用户名可能已存在").dump()));
    }
}

void AuthService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["userid"].get<int>();
    // 更新用户状态信息
    User user = _userModel.getUserById(userid);
    // 取消订阅用户的channel
    _redis.unsubscribe(userid);
    user.setState("offline");
    _userModel.updateUserInfo(user);
    // 从在线用户管理器中移除用户
    _onlineUserManager.removeUser(userid);
}
