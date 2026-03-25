#include "chatService.hpp"

#include "userModel.hpp"
#include "offLineMsgModel.hpp"
#include "friendModel.hpp"
#include "groupModel.hpp"
#include "redis.hpp"
#include "public.hpp"
#include <nlohmann/json.hpp>
#include <mutex>
#include <unordered_map>

using namespace std;
using json = nlohmann::json;
using namespace muduo::net;
using namespace muduo;

void ChatService::handleMessage(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int msgid = js["msgid"].get<int>();
    switch (msgid)
    {
    case ONE_CHAT_MSG:
        oneChat(conn, js, time);
        break;
    case GROUP_CHAT_MSG:
        groupChat(conn, js, time);
        break;
    default:
        break;
    }
}

// 构造函数
ChatService::ChatService(UserModel *userModel, OffLineMsgModel *offLineMsgModel,
                         FriendModel *friendModel, GroupModel *groupModel,
                         Redis *redis, OnlineUserManager *onlineUserManager)
{
    _userModel = userModel;
    _offLineMsgModel = offLineMsgModel;
    _friendModel = friendModel;
    _groupModel = groupModel;
    _redis = redis;
    _onlineUserManager = onlineUserManager;
}
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();
    cout << "toid: " << toid << endl;
    User touser = _userModel->getUserById(toid);
    if (touser.getId() == -1)
    {
        // 目标用户不存在
        cout << "目标用户不存在" << endl;
        json response;
        response["msgid"] = ONE_CHAT_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "目标用户不存在，消息发送失败";
        conn->send(response.dump());
        return;
    }
    if (touser.getState() == "online")
    {
        // 检查目标用户是否在本服务器登录
        TcpConnectionPtr toConn = _onlineUserManager->getUserConn(toid);
        if (toConn != nullptr)
        {
            // 在本服务器找到toid用户的连接，转发消息
            cout << "在本服务器找到toid用户的连接，转发消息" << endl;
            toConn->send(js.dump());
            // 回复userid用户，消息发送成功
            json response;
            response["msgid"] = ONE_CHAT_MSG_ACK;
            response["errno"] = 0;
            response["errmsg"] = "消息发送成功";
            conn->send(response.dump());
        }
        else
        {
            int ret = _redis->publish(toid, js.dump());
            // 目标用户在线，通过Redis发布消息
            cout << "在本服务器未找到toid用户的连接，通过Redis发布消息" << endl;
            if (ret != 0)
            {
                // 发布失败
                json response;
                response["msgid"] = ONE_CHAT_MSG_ACK;
                response["errno"] = 1;
                response["errmsg"] = "消息发送失败";
                conn->send(response.dump());
                return;
            }
            // 回复userid用户，消息发送成功
            json response;
            response["msgid"] = ONE_CHAT_MSG_ACK;
            response["errno"] = 0;
            response["errmsg"] = "消息发送成功";
            conn->send(response.dump());
        }
    }
    else
    {
        // 目标用户不在线，存储离线消息
        cout << "用户不在线，存储离线消息" << endl;
        json response;
        response["msgid"] = ONE_CHAT_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户不在线，发送离线消息";
        conn->send(response.dump());
        _offLineMsgModel->insert(toid, js.dump());
    }
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["userid"].get<int>();
    int groupid = js["groupid"].get<int>();
    string msg = js["msg"];
    vector<groupUser> users = _groupModel->groupUsers(groupid);
    {
        for (auto &groupuser : users)
        {
            if (groupuser.getId() != userid)
            {
                if (groupuser.getState() == "online")
                {
                    TcpConnectionPtr userConn = _onlineUserManager->getUserConn(groupuser.getId());
                    if (userConn != nullptr)
                    {
                        // 找到用户连接，转发消息
                        userConn->send(js.dump());
                        continue;
                    }
                    _redis->publish(groupuser.getId(), js.dump());
                }
                else
                {
                    // 用户不在线，存储离线消息
                    _offLineMsgModel->insert(groupuser.getId(), js.dump());
                }
            }
        }
    }
}
