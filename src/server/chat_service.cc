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

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的处理业务
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        return nullptr;
    }
    else
    {
        return it->second;
    }
}