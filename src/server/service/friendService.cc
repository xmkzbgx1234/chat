#include "friendService.hpp"
#include "UserModel.hpp"
#include "friendModel.hpp"
#include "redis.hpp"
#include "public.hpp"
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
    int userid = js["userid"].get<int>();
    int friendid = js["friendid"].get<int>();
    // 存储好友信息
    if (_userModel.getUserById(friendid).getId() != -1)
    {
        // LOG_INFO << "添加好友：" << userid << " -> " << friendid;
        _friendModel.insert(userid, friendid);
        json response;
        response["msgid"] = ADD_FRIEND_MSG_ACK;
        response["errno"] = 0;
        response["errmsg"] = "添加好友成功";
        conn->send(response.dump());
    }
}
