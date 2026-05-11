#include "friendService.hpp"
#include "UserModel.hpp"
#include "friendModel.hpp"
#include "redis.hpp"
#include "public.hpp"
#include "validator.hpp"
#include "errorCode.hpp"
#include "responseBuilder.hpp"
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <muduo/net/TcpConnection.h>

using namespace std;
using json = nlohmann::json;
using namespace muduo::net;
using namespace muduo;

FriendService::FriendService(UserModel &userModel, FriendModel &friendModel)
    : _userModel(userModel), _friendModel(friendModel)
{}

// 处理消息
void FriendService::handleMessage(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int msgid = js["msgid"].get<int>();
    switch (msgid)
    {
    case ADD_FRIEND_MSG:
        addFriend(conn, js, time);
        break;
    default:
        break;
    }
}

void FriendService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = Validator::getInt(js, "userid", -1);
    int friendid = Validator::getInt(js, "friendid", -1);
    
    if (!Validator::isValidUserId(userid)) {
        conn->send(encodeMessage(ResponseBuilder::error(ADD_FRIEND_MSG_ACK, ErrorCode::INVALID_USER_ID).dump()));
        return;
    }
    
    if (!Validator::isValidUserId(friendid)) {
        conn->send(encodeMessage(ResponseBuilder::error(ADD_FRIEND_MSG_ACK, ErrorCode::INVALID_USER_ID, "无效的好友ID").dump()));
        return;
    }
    
    if (userid == friendid) {
        conn->send(encodeMessage(ResponseBuilder::error(ADD_FRIEND_MSG_ACK, ErrorCode::FRIEND_CANNOT_ADD_SELF).dump()));
        return;
    }
    
    User friendUser = _userModel.getUserById(friendid);
    if (friendUser.getId() == -1) {
        conn->send(encodeMessage(ResponseBuilder::error(ADD_FRIEND_MSG_ACK, ErrorCode::FRIEND_USER_NOT_EXIST).dump()));
        return;
    }
    
    _friendModel.insert(userid, friendid);
    conn->send(encodeMessage(ResponseBuilder::success(ADD_FRIEND_MSG_ACK, "添加好友成功").dump()));
}
