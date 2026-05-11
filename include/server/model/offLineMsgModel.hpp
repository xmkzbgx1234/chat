#ifndef OFFLINE_MSG_MODEL_HPP
#define OFFLINE_MSG_MODEL_HPP

#include <vector>
#include <string>

// 离线消息数据操作类
class OffLineMsgModel
{
public:
    // 存储离线消息
    void insert(int id, const std::string &msg);
    // 获取离线消息
    std::vector<std::string> get(int userId);
    // 删除离线消息
    void remove(int userId);
};

#endif // OFFLINE_MSG_MODEL_HPP