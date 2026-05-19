#ifndef GROUPUSER_HPP
#define GROUPUSER_HPP

#include <string>
#include "user.hpp"
using namespace std;

class groupUser : public User
{
private:
    string role; // 1-群主 2-管理员 3-普通成员
public:
    groupUser(){};
    groupUser(int userid_, const string &username, const string &userstate, const string &grouprole)
        : User(userid_, username, "", userstate) // User构造：password传空字符串（无意义）
    {
        this->role = grouprole; // 手动给groupUser自身的role赋值（关键！）
    }
    void setRole(const string &role){
        this->role = role;
    }
    string getRole() const{
        return this->role;
    }
};

#endif // GROUPUSER_HPP