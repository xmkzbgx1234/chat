#ifndef REDIS_HPP
#define REDIS_HPP

#include <hiredis/hiredis.h>
#include <functional>
#include <string>
#include <iostream>

class Redis
{
public:
    Redis();
    ~Redis();

    // 连接Redis服务器
    bool connect();
    // 连接Redis服务器（带密码）
    bool connect(const std::string &ip, int port, const std::string &pass);
    // 向指定channel发布Redis消息
    bool publish(const int channel, const std::string &message);
    // 订阅Redis频道
    bool subscribe(const int channel);
    // 取消订阅Redis频道
    bool unsubscribe(const int channel);
    // 在独立线程中处理订阅消息
    void observer_channel_message();
    // 初始化订阅消息回调函数
    void initNotifyMessageHandler(std::function<void(int, const std::string &)> handler);


private:
    redisContext *_publish_context; // Redis上下文
    redisContext *_subscribe_context; // Redis上下文

    // 订阅消息回调函数,上报service层
    std::function<void(int, const std::string &)> notify_message_handler;

    // 检查连接是否成功
    bool checkConnect(redisContext *context, const std::string &type);
    // 检查Redis命令回复是否有效
    bool checkReply(redisReply* reply, const std::string& cmd);

    std::string _redis_ip; // Redis IP地址
    int _redis_port; // Redis端口号
    std::string _redis_pass; // Redis密码
};


#endif // REDIS_HPP