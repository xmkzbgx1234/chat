#ifndef GROUPMODEL_HPP
#define GROUPMODEL_HPP

#include <vector>
#include "group.hpp"

class GroupModel{
public:
    // 创建群聊
    bool createGroup(Group &group);
    // 加入群聊
    bool addGroup(int userid, int groupid, string role);
    // 查询用户所在群组信息
    std::vector<Group> queryGroups(int userid);
    // 根据群组id查看群聊中所有用户
    std::vector<groupUser> groupUsers(int groupid);
 
private:

};

#endif 