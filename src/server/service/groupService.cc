#include "groupService.hpp"
#include "groupModel.hpp"
#include <muduo/net/TcpConnection.h>
#include <nlohmann/json.hpp>
#include "public.hpp"
#include "validator.hpp"
#include "responseBuilder.hpp"

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
    
    if (!Validator::isValidUserId(userid)) {
        conn->send(encodeMessage(ResponseBuilder::error(CREATE_GROUP_MSG_ACK, ErrorCode::INVALID_PARAM, "无效的用户ID").dump()));
        return;
    }
    
    if (!Validator::isValidGroupName(groupname)) {
        conn->send(encodeMessage(ResponseBuilder::error(CREATE_GROUP_MSG_ACK, ErrorCode::INVALID_PARAM, "群名不能为空且长度不能超过128个字符").dump()));
        return;
    }
    
    Group group = Group(-1, groupname, groupdesc);
    if(_groupModel.createGroup(group)){
        if(_groupModel.addGroup(userid, group.getId(), "creator")){
            json response = ResponseBuilder::success(CREATE_GROUP_MSG_ACK, "创建群组成功");
            response["groupid"] = group.getId();
            response["groupname"] = group.getName();
            response["groupdesc"] = group.getDesc();
            conn->send(encodeMessage(response.dump()));
        }
        else
        {
            _groupModel.deleteGroup(group.getId());
            conn->send(encodeMessage(ResponseBuilder::error(CREATE_GROUP_MSG_ACK, ErrorCode::DB_ERROR, "创建群组失败，无法添加创建者").dump()));
        }
    }
    else{
        conn->send(encodeMessage(ResponseBuilder::error(CREATE_GROUP_MSG_ACK, ErrorCode::DB_ERROR, "创建群组失败").dump()));
    }
}

void GroupService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = Validator::getInt(js, "userid", -1);
    int groupid = Validator::getInt(js, "groupid", -1);
    
    if (!Validator::isValidUserId(userid)) {
        conn->send(encodeMessage(ResponseBuilder::error(ADD_GROUP_MSG_ACK, ErrorCode::INVALID_PARAM, "无效的用户ID").dump()));
        return;
    }
    
    if (!Validator::isValidGroupId(groupid)) {
        conn->send(encodeMessage(ResponseBuilder::error(ADD_GROUP_MSG_ACK, ErrorCode::INVALID_PARAM, "无效的群组ID").dump()));
        return;
    }
    
    if(_groupModel.addGroup(userid, groupid, "normal")){
        conn->send(encodeMessage(ResponseBuilder::success(ADD_GROUP_MSG_ACK, "加入群组成功").dump()));
    }
    else{
        conn->send(encodeMessage(ResponseBuilder::error(ADD_GROUP_MSG_ACK, ErrorCode::DB_ERROR, "加入群组失败，群组可能不存在或您已在群组中").dump()));
    }
}
