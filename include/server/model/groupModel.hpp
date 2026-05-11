#ifndef GROUPMODEL_HPP
#define GROUPMODEL_HPP

#include <vector>
#include "group.hpp"

class GroupModel{
public:
    bool createGroup(Group &group);
    bool addGroup(int userid, int groupid, string role);
    std::vector<Group> queryGroups(int userid);
    std::vector<groupUser> groupUsers(int groupid);
    bool deleteGroup(int groupid);
  
private:

};

#endif 