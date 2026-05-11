#include "groupService.hpp"
#include "groupModel.hpp"
#include <muduo/net/TcpConnection.h>
#include <nlohmann/json.hpp>
#include "public.hpp"
#include "validator.hpp"

using namespace muduo;
using namespace muduo::net;
using namespace nlohmann;


GroupService::GroupService(GroupModel& groupModel)
    : _groupModel(groupModel)
{}

void GroupService::handleMessage(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int msgid = js["msgid"].get<int>();
    switch(msgid){
        case CREATE_GROUP_MSG:
            createGroup(conn, js, time);
            break;
        case ADD_GROUP_MSG:
            addGroup(conn, js, time);
            break;
        default:
            break;
    }
}



void GroupService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = Validator::getInt(js, "userid", -1);
    string groupname = Validator::getString(js, "groupname", "");
    string groupdesc = Validator::getString(js, "groupdesc", "");
    
    // 参数校验
    if (!Validator::isValidUserId(userid)) {
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "无效的用户ID";
        conn->send(encodeMessage(response.dump()));
        return;
    }
    
    if (!Validator::isValidGroupName(groupname)) {
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "群名不能为空且长度不能超过128个字符";
        conn->send(encodeMessage(response.dump()));
        return;
    }
    
    // 创建群组
    Group group = Group(-1, groupname, groupdesc);
    if(_groupModel.createGroup(group)){
        // 群组创建成功
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["errno"] = 0;
        response["errmsg"] = "创建群组成功";
        response["groupid"] = group.getId();
        response["groupname"] = group.getName();
        response["groupdesc"] = group.getDesc();
        // 加入群组
        _groupModel.addGroup(userid, group.getId(), "creator");
        conn->send(encodeMessage(response.dump()));
    }
    else{
        // 群组创建失败
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "创建群组失败";
        conn->send(encodeMessage(response.dump()));
    }
}

void GroupService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = Validator::getInt(js, "userid", -1);
    int groupid = Validator::getInt(js, "groupid", -1);
    
    // 参数校验
    if (!Validator::isValidUserId(userid)) {
        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "无效的用户ID";
        conn->send(encodeMessage(response.dump()));
        return;
    }
    
    if (!Validator::isValidGroupId(groupid)) {
        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "无效的群组ID";
        conn->send(encodeMessage(response.dump()));
        return;
    }
    
    // 加入群组
    if(_groupModel.addGroup(userid, groupid, "normal")){
        // 加入群组成功
        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["errno"] = 0;
        response["errmsg"] = "加入群组成功";
        conn->send(encodeMessage(response.dump()));
    }
    else{
        // 加入群组失败
        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "加入群组失败，群组可能不存在或您已在群组中";
        conn->send(encodeMessage(response.dump()));
    }
}
