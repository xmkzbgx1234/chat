#ifndef PUBLIC_HPP
#define PUBLIC_HPP

/*
server 和client 都需要用到的一些公共文件放在这里
*/
enum EnMsgType
{
    LOGIN_MSG = 1, // 登录消息
    LOGIN_MSG_ACK, // 登录响应消息
    REG_MSG,       // 注册消息
    REG_MSG_ACK,   // 注册响应消息
};

#endif // PUBLIC_HPP