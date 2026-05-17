#ifndef PUBLIC_HPP
#define PUBLIC_HPP

#include <arpa/inet.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

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
    GROUP_CHAT_MSG_ACK, // 群组聊天响应消息14
    LOGINOUT_MSG, // 退出登录消息15
    KICK_OFF_MSG, // 踢下线通知消息16

    // 游戏消息 20-32
    GAME_CREATE_ROOM = 20,    // 创建房间
    GAME_JOIN_ROOM,           // 加入房间21
    GAME_LEAVE_ROOM,          // 离开房间22
    GAME_ROOM_LIST,           // 房间列表23
    GAME_ROOM_STATE,          // 房间状态广播24
    GAME_READY,               // 准备就绪25
    GAME_START,               // 游戏开始26
    GAME_KEY_PRESS,           // 玩家按键27
    GAME_HIT_RESULT,          // 命中结果28
    GAME_APPLE_SPAWN,         // 苹果生成29
    GAME_SCORE_UPDATE,        // 分数更新30
    GAME_OPPONENT_STATE,      // 对手状态同步31
    GAME_OVER,                // 游戏结束32
    GAME_LEADERBOARD,         // 排行榜请求33
    GAME_LEADERBOARD_ACK,     // 排行榜响应34
    GAME_OPPONENT_HIT,        // 对手命中通知35
};

static const std::size_t header_size = sizeof(uint32_t);
static const uint32_t max_body_size = 1024 * 1024;

inline std::string encodeMessage(const std::string &body)
{
    if (body.size() > max_body_size)
    {
        throw std::length_error("message body too large");
    }

    const uint32_t bodySize = static_cast<uint32_t>(body.size());
    const uint32_t networkSize = htonl(bodySize);

    std::string packet(header_size + body.size(), '\0');
    std::memcpy(&packet[0], &networkSize, header_size);
    std::memcpy(&packet[header_size], body.data(), body.size());
    return packet;
}

inline bool tryDecodeMessages(std::string &buffer, std::vector<std::string> &messages)
{
    while (buffer.size() >= header_size)
    {
        uint32_t networkSize = 0;
        std::memcpy(&networkSize, buffer.data(), header_size);
        const uint32_t bodySize = ntohl(networkSize);
        if (bodySize > max_body_size)
        {
            return false;
        }
        if (buffer.size() < header_size + bodySize)
        {
            break;
        }

        messages.push_back(buffer.substr(header_size, bodySize));
        buffer.erase(0, header_size + bodySize);
    }
    return true;
}
 
#endif // PUBLIC_HPP
