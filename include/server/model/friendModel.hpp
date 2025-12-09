#ifndef FRIEND_MODEL_HPP
#define FRIEND_MODEL_HPP

#include <vector>
#include "user.hpp"

class FriendModel
{
public:
    FriendModel();
    // 添加好友
    void insert(int userid, int friendid);
    // 获取好友列表
    std::vector<User> query(int userid);
private:
};


#endif // FRIEND_MODEL_HPP