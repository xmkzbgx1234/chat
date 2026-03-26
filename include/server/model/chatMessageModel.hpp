#ifndef CHAT_MESSAGE_MODEL_HPP
#define CHAT_MESSAGE_MODEL_HPP

#include <cstddef>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class ChatMessageModel
{
public:
    ChatMessageModel();

    bool initTable();
    long long insertSingleMessage(int fromUserId, int toUserId, const std::string &content, const std::string &msgTime);
    long long insertGroupMessage(int fromUserId, int groupId, const std::string &content, const std::string &msgTime);
    std::vector<nlohmann::json> queryUserMessages(int userId, long long lastMessageId, std::size_t limit = 200);
};

#endif
