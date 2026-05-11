#include "redis.hpp"
#include "log.h"
#include <thread>

using namespace std;

Redis::Redis(){
    _publish_context = nullptr;
    _subscribe_context = nullptr;
}

Redis::~Redis(){
    if(_publish_context != nullptr){
        redisFree(_publish_context);
    }
    if(_subscribe_context != nullptr){
        redisFree(_subscribe_context);
    }
}


bool Redis::connect(const string& ip, int port, const string& pass) {
    _redis_ip = ip;
    _redis_port = port;
    _redis_pass = pass;

    // 1. 创建发布连接 + 认证
    _publish_context = redisConnect(ip.c_str(), port);
    if (!checkConnect(_publish_context, "publish")) {
        redisFree(_publish_context);
        _publish_context = nullptr;
        return false;
    }

    // 2. 创建订阅连接 + 认证（订阅必须独立连接，不能和发布共用）
    _subscribe_context = redisConnect(ip.c_str(), port);
    if (!checkConnect(_subscribe_context, "subscribe")) {
        redisFree(_publish_context);
        _publish_context = nullptr;
        return false;
    }

    // 3. 启动独立线程监听订阅消息（避免阻塞主线程）
    thread t(&Redis::observer_channel_message, this);
    t.detach();  // 分离线程，后台运行

    LOG_INFO << "Redis publish/subscribe connection established";
    return true;
}

bool Redis::connect() {
    // 兼容原有无密码连接接口
    return connect("127.0.0.1", 6379, "");
}

bool Redis::checkConnect(redisContext* ctx, const string& type) {
    // 检查Redis连接是否有效（含密码认证）
    if (ctx == nullptr) {
        LOG_ERROR << "Redis " << type << " connection failed: null context";
        return false;
    }
    if (ctx->err) {
        LOG_ERROR << "Redis " << type << " connection failed: " << ctx->errstr;
        redisFree(ctx);
        return false;
    }

    // 密码认证（非空则执行）
    if (!_redis_pass.empty()) {
        redisReply* reply = (redisReply*)redisCommand(ctx, "AUTH %s", _redis_pass.c_str());
        if (!checkReply(reply, "AUTH")) {
            redisFree(ctx);
            return false;
        }
        freeReplyObject(reply);
    }

    return true;
}

bool Redis::checkReply(redisReply* reply, const string& cmd) {
    // 检查Redis命令回复是否有效
    if (reply == nullptr) {
        LOG_ERROR << "Redis command " << cmd << " failed: empty reply";
        return false;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        LOG_ERROR << "Redis command " << cmd << " failed: "
                  << (reply->str != nullptr ? reply->str : "unknown error");
        freeReplyObject(reply);
        return false;
    }
    return true;
}

bool Redis::publish(int channel, const string& message)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    LOG_DEBUG << "publish channel=" << channel << ", message=" << message;
    if (nullptr == reply)
    {
        LOG_ERROR << "publish command failed";
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool Redis::subscribe(const int channel){
    if(_subscribe_context == nullptr){
        LOG_ERROR << "Redis subscribe context is null";
        return false;
    }
    // 安全格式化命令：避免特殊字符导致的命令错误
    redisReply *reply = (redisReply *)redisCommand(_subscribe_context, "SUBSCRIBE %d", channel);
    if(!checkReply(reply, "SUBSCRIBE")){
        return false;
    }
    freeReplyObject(reply);
    LOG_INFO << "subscribe channel=" << channel;
    return true;
}

bool Redis::unsubscribe(const int channel){
    if(_subscribe_context == nullptr){
        LOG_ERROR << "Redis unsubscribe context is null";
        return false;
    }
    // 安全格式化命令：避免特殊字符导致的命令错误
    char cmd[1024] = {0};
    snprintf(cmd, sizeof(cmd), "UNSUBSCRIBE %d", channel);
    redisReply *reply = (redisReply *)redisCommand(_subscribe_context, cmd);
    if(!checkReply(reply, cmd)){
        return false;
    }
    freeReplyObject(reply);
    return true;
}

void Redis::observer_channel_message() {
    // 独立线程监听订阅消息
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(_subscribe_context, (void **)&reply)) {
        // 1. 基础校验：reply有效、是数组类型、至少有3个元素
        if (reply == nullptr || 
            reply->type != REDIS_REPLY_ARRAY || 
            reply->elements < 3) {
            freeReplyObject(reply);
            continue;
        }

        // 2. 获取消息类型（element[0]），仅处理"message"类型
        redisReply *msg_type_reply = reply->element[0];
        if (msg_type_reply == nullptr || 
            msg_type_reply->type != REDIS_REPLY_STRING || 
            msg_type_reply->str == nullptr) {
            freeReplyObject(reply);
            continue;
        }
        std::string msg_type = msg_type_reply->str;
        if (msg_type != "message") {
            // 非业务消息（如subscribe/unsubscribe），可忽略或打日志
            freeReplyObject(reply);
            continue;
        }

        // 3. 校验频道（element[1]）：必须是有效字符串
        redisReply *channel_reply = reply->element[1];
        if (channel_reply == nullptr || 
            channel_reply->type != REDIS_REPLY_STRING || 
            channel_reply->str == nullptr) {
            freeReplyObject(reply);
            continue;
        }

        // 4. 校验消息内容（element[2]）：必须是有效字符串
        redisReply *content_reply = reply->element[2];
        if (content_reply == nullptr || 
            content_reply->type != REDIS_REPLY_STRING || 
            content_reply->str == nullptr) {
            freeReplyObject(reply);
            continue;
        }

        // 5. 安全调用消息处理器
        int channel = atoi(channel_reply->str);
        std::string content = content_reply->str;
        notify_message_handler(channel, content);

        // 释放reply资源
        freeReplyObject(reply);
    }
}

void Redis::initNotifyMessageHandler(std::function<void(int, const std::string &)> handler){
    notify_message_handler = handler;
}
