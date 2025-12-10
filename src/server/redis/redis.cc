#include "redis.hpp"
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

    cout << "Redis 发布/订阅连接成功" << endl;
    return true;
}

// 兼容原有无密码连接接口
bool Redis::connect() {
    return connect("127.0.0.1", 6379, "");
}

// ========== 私有工具函数（核心：错误处理+重连） ==========
// 检查Redis连接是否有效（含密码认证）
bool Redis::checkConnect(redisContext* ctx, const string& type) {
    if (ctx == nullptr) {
        cerr << "Redis " << type << " 连接失败：空指针" << endl;
        return false;
    }
    if (ctx->err) {
        cerr << "Redis " << type << " 连接失败：" << ctx->errstr << endl;
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

// 检查Redis命令回复是否有效
bool Redis::checkReply(redisReply* reply, const string& cmd) {
    if (reply == nullptr) {
        cerr << "Redis 执行 " << cmd << " 失败：回复为空" << endl;
        return false;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        cerr << "Redis 执行 " << cmd << " 失败：" << reply->str << endl;
        freeReplyObject(reply);
        return false;
    }
    return true;
}

bool Redis::publish(int channel, const string& message)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    cout << "channel: " << channel << ", message: " << message << endl;
    if (nullptr == reply)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool Redis::subscribe(const int channel){
    if(_subscribe_context == nullptr){
        cerr << "Redis subscribe context is null" << endl;
        return false;
    }
    // 安全格式化命令：避免特殊字符导致的命令错误
    redisReply *reply = (redisReply *)redisCommand(_subscribe_context, "SUBSCRIBE %d", channel);
    if(!checkReply(reply, "SUBSCRIBE")){
        return false;
    }
    freeReplyObject(reply);
    cout << "subscribe channel: " << channel << endl;
    return true;
}

bool Redis::unsubscribe(const int channel){
    if(_subscribe_context == nullptr){
        cerr << "Redis unsubscribe context is null" << endl;
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

// 独立线程监听订阅消息
void Redis::observer_channel_message(){
    redisReply *reply = nullptr;
    while(REDIS_OK == redisGetReply(_subscribe_context, (void **)&reply)){
        if(reply != nullptr){
            if(reply->element[2] != nullptr){
                notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
            }
            freeReplyObject(reply);
        }
    }
}

void Redis::initNotifyMessageHandler(std::function<void(int, const std::string &)> handler){
    notify_message_handler = handler;
}
