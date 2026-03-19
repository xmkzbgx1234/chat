#ifndef PUBLIC_HPP
#define PUBLIC_HPP

/*
server 和client 都需要用到的一些公共文件放在这里
*/
enum EnMsgType
{
    LOGIN_MSG = 1, // 登录消息1
    LOGIN_MSG_ACK, // 登录响应消息2
    REG_MSG,       // 注册消息3
    REG_MSG_ACK,   // 注册响应消息4
    ONE_CHAT_MSG,  // 点对点聊天消息5
    ONE_CHAT_MSG_ACK, // 点对点聊天响应消息6
    ADD_FRIEND_MSG, // 添加好友消息7
    ADD_FRIEND_MSG_ACK, // 添加好友响应消息8
    CREATE_GROUP_MSG, // 创建群组消息9
    CREATE_GROUP_MSG_ACK, // 创建群组响应消息10
    ADD_GROUP_MSG,    // 加入群组消息11
    ADD_GROUP_MSG_ACK, // 加入群组响应消息12
    GROUP_CHAT_MSG,   // 群组聊天消息13
    LOGINOUT_MSG, // 退出登录消息14
};

#endif // PUBLIC_HPP