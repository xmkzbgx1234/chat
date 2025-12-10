#include <muduo/net/TcpConnection.h>
#include <muduo/base/Logging.h>
#include "chat_service.hpp"
#include <iostream>
#include "public.hpp"
#include "User.hpp"

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService()
{
    // 注册消息以及对应的Handler回调操作
    _msgHandlerMap.insert({1, std::bind(&ChatService::login, this,
                                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({2, std::bind(&ChatService::regist, this,
                                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({3, std::bind(&ChatService::oneChat, this,
                                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({4, std::bind(&ChatService::addFriend, this,
                                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({5, std::bind(&ChatService::createGroup, this,
                                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({6, std::bind(&ChatService::addGroup, this,
                                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({7, std::bind(&ChatService::groupChat, this,
                                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _userModel = UserModel();
}

// 处理登录业务
void ChatService::login(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time)
{
    std::cout << "do login service" << std::endl;
    // 具体的业务处理
    int id = js["id"].get<int>();
    std::string password = js["password"];
    User user = _userModel.getUserById(id);
    if (user.getId() != -1 && user.getPassword() == password)
    {
        if(user.getState() == "online")
        {
            // 用户已经登录，不能重复登录
            nlohmann::json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["error"] = 2; // 2表示用户已经登录
            response["errmsg"] = "该账号已登陆，不能重复登录";
            conn->send(response.dump());
            return;
        }
        // 登录成功
        // 更新用户状态信息
        user.setState("online");
        _userModel.updateUserInfo(user);
        nlohmann::json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["error"] = 0;
        response["name"] = user.getName();
        conn->send(response.dump());
    }
    else
    {
        // 登录失败
        nlohmann::json response;
        response["msgid"] = LOGIN_MSG;
        response["error"] = 1;
        response["errmsg"] = "用户名或密码错误";
        conn->send(response.dump());
    }
}

// 处理注册业务
void ChatService::regist(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time)
{
    std::cout << "do regist service" << std::endl;
    // 具体的业务处理
    std::string name = js["name"];
    std::string password = js["password"];
    User user = User();
    user.setName(name);
    user.setPassword(password);
    bool status = _userModel.insert(user);
    if (status)
    {
        // 注册成功
        nlohmann::json response;
        response["msgid"] = REG_MSG_ACK;
        response["error"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        nlohmann::json response;
        response["msgid"] = REG_MSG_ACK;
        response["error"] = 1;
        conn->send(response.dump());
    }

}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["userid"].get<int>();
    int friendid = js["friendid"].get<int>();
    // 存储好友信息
    if(_userModel.getUserById(friendid).getId() != -1){
        LOG_INFO << "添加好友：" << userid << " -> " << friendid;
        _friendModel.insert(userid, friendid);
        json response;
        response["msgid"] = ADD_FRIEND_MSG_ACK;
        response["errno"] = 0;
        response["errmsg"] = "添加好友成功";
        conn->send(response.dump());
    }
}

void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["userid"].get<int>();
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];
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
        conn->send(response.dump());
    }
    else{
        // 群组创建失败
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "创建群组失败";
        conn->send(response.dump());
    }
}

void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["userid"].get<int>();
    int groupid = js["groupid"].get<int>();
    // 加入群组
    if(_groupModel.addGroup(userid, groupid, "normal")){
        // 加入群组成功
        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["errno"] = 0;
        response["errmsg"] = "加入群组成功";
        conn->send(response.dump());
    }
    else{
        // 加入群组失败
        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "加入群组失败";
        conn->send(response.dump());
    }
}

void ChatService::handleRedisMessage(int channel, const string &message){
    lock_guard<mutex> lock(_connMutex);
    cout << "handleRedisMessage: " << "channel: " << channel << ", message: " << message << endl;
    auto it = _userConnMap.find(channel);
    if (it != _userConnMap.end())
    {
        it->second->send(message);
        return;
    }

    // 存储该用户的离线消息
    _offLineMsgModel.insert(channel, message);
}



