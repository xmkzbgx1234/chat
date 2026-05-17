#include "chatService.hpp"

#include "UserModel.hpp"
#include "offLineMsgModel.hpp"
#include "groupModel.hpp"
#include "redis.hpp"
#include "public.hpp"
#include "log.h"
#include "validator.hpp"
#include "responseBuilder.hpp"
#include "errorCode.hpp"
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

ChatService::ChatService(UserModel &userModel, OffLineMsgModel &offLineMsgModel,
                         GroupModel &groupModel, ChatMessageModel &chatMessageModel,
                         Redis &redis, OnlineUserManager &onlineUserManager)
    : _userModel(userModel),
      _offLineMsgModel(offLineMsgModel),
      _groupModel(groupModel),
      _chatMessageModel(chatMessageModel),
      _redis(redis),
      _onlineUserManager(onlineUserManager)
{}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = Validator::getInt(js, "toid", -1);
    int fromid = Validator::getInt(js, "id", -1);
    string msg = Validator::getString(js, "msg", "");
    string msgTime = Validator::getString(js, "time", "");
    
    if (!Validator::isValidUserId(toid)) {
        conn->send(encodeMessage(ResponseBuilder::error(ONE_CHAT_MSG_ACK, ErrorCode::INVALID_USER_ID, "无效的目标用户ID").dump()));
        return;
    }
    
    if (!Validator::isValidUserId(fromid)) {
        conn->send(encodeMessage(ResponseBuilder::error(ONE_CHAT_MSG_ACK, ErrorCode::INVALID_USER_ID, "无效的发送者ID").dump()));
        return;
    }
    
    if (!Validator::isValidMessage(msg)) {
        conn->send(encodeMessage(ResponseBuilder::error(ONE_CHAT_MSG_ACK, ErrorCode::INVALID_MESSAGE, "消息内容不能为空且不能超过6000个字符").dump()));
        return;
    }
    
    LOG_DEBUG << "oneChat target user=" << toid;
    User touser = _userModel.getUserById(toid);
    if (touser.getId() == -1)
    {
        LOG_WARN << "oneChat target user does not exist: " << toid;
        conn->send(encodeMessage(ResponseBuilder::error(ONE_CHAT_MSG_ACK, ErrorCode::MSG_TARGET_NOT_FOUND, "目标用户不存在，消息发送失败").dump()));
        return;
    }
    long long messageId = _chatMessageModel.insertSingleMessage(fromid, toid, msg, msgTime);
    if (messageId < 0)
    {
        conn->send(encodeMessage(ResponseBuilder::error(ONE_CHAT_MSG_ACK, ErrorCode::DB_ERROR, "消息持久化失败").dump()));
        return;
    }
    js["messageid"] = messageId;
    js["session_type"] = "single";
    js["fromid"] = fromid;
    if (touser.getState() == "online")
    {
        TcpConnectionPtr toConn = _onlineUserManager.getUserConn(toid);
        if (toConn != nullptr)
        {
            LOG_INFO << "Forward oneChat to local connection, toid=" << toid;
            toConn->send(encodeMessage(js.dump()));
            json response = ResponseBuilder::success(ONE_CHAT_MSG_ACK, "消息发送成功");
            response["message"] = js;
            conn->send(encodeMessage(response.dump()));
        }
        else
        {
            int ret = _redis.publish(toid, js.dump());
            LOG_INFO << "Forward oneChat through Redis, toid=" << toid;
            if (ret != 0)
            {
                conn->send(encodeMessage(ResponseBuilder::error(ONE_CHAT_MSG_ACK, ErrorCode::NET_DISCONNECTED, "消息发送失败").dump()));
                return;
            }
            json response = ResponseBuilder::success(ONE_CHAT_MSG_ACK, "消息发送成功");
            response["message"] = js;
            conn->send(encodeMessage(response.dump()));
        }
    }
    else
    {
        LOG_INFO << "Store offline oneChat message, toid=" << toid;
        json response = ResponseBuilder::success(ONE_CHAT_MSG_ACK, "用户不在线，发送离线消息");
        response["message"] = js;
        conn->send(encodeMessage(response.dump()));
        _offLineMsgModel.insert(toid, js.dump());
    }
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = Validator::getInt(js, "userid", -1);
    int groupid = Validator::getInt(js, "groupid", -1);
    string msg = Validator::getString(js, "msg", "");
    
    if (!Validator::isValidUserId(userid)) {
        conn->send(encodeMessage(ResponseBuilder::error(GROUP_CHAT_MSG_ACK, ErrorCode::INVALID_USER_ID, "无效的用户ID").dump()));
        return;
    }
    
    if (!Validator::isValidGroupId(groupid)) {
        conn->send(encodeMessage(ResponseBuilder::error(GROUP_CHAT_MSG_ACK, ErrorCode::INVALID_GROUP_ID, "无效的群组ID").dump()));
        return;
    }
    
    if (!Validator::isValidMessage(msg)) {
        conn->send(encodeMessage(ResponseBuilder::error(GROUP_CHAT_MSG_ACK, ErrorCode::INVALID_MESSAGE, "消息内容不能为空且不能超过6000个字符").dump()));
        return;
    }
    
    long long messageId = _chatMessageModel.insertGroupMessage(userid, groupid, msg, js["time"]);
    if (messageId < 0)
    {
        conn->send(encodeMessage(ResponseBuilder::error(GROUP_CHAT_MSG_ACK, ErrorCode::DB_ERROR, "群消息持久化失败").dump()));
        return;
    }
    js["messageid"] = messageId;
    js["session_type"] = "group";
    js["fromid"] = userid;
    vector<groupUser> users = _groupModel.groupUsers(groupid);
    {
        for (groupUser &groupuser : users)
        {
            if (groupuser.getId() != userid)
            {
                if (groupuser.getState() == "online")
                {
                    TcpConnectionPtr userConn = _onlineUserManager.getUserConn(groupuser.getId());
                    if (userConn != nullptr)
                    {
                        userConn->send(encodeMessage(js.dump()));
                        continue;
                    }
                    _redis.publish(groupuser.getId(), js.dump());
                }
                else
                {
                    _offLineMsgModel.insert(groupuser.getId(), js.dump());
                }
            }
        }
    }
    json response = ResponseBuilder::success(GROUP_CHAT_MSG_ACK, "群消息发送成功");
    response["message"] = js;
    conn->send(encodeMessage(response.dump()));
}
