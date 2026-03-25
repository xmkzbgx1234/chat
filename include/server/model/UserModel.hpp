#ifndef USER_MODEL_HPP
#define USER_MODEL_HPP

#include "user.hpp"
#include "db.hpp"

// User表的数据操作类
class UserModel
{
public:
    UserModel();

    // 创建用户
    bool insert(User &user);
    // 根据用户ID获取用户信息
    User getUserById(int id);

    // 更新用户信息
    bool updateUserInfo(const User &user);

    // 删除用户
    bool deleteUser(int id);

    // 重置用户状态
    void resetState();
private:

};

#endif // USER_MODEL_HPP