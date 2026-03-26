#ifndef USER_MODEL_HPP
#define USER_MODEL_HPP

#include "db.hpp"
#include "user.hpp"

class UserModel
{
public:
    UserModel();
    bool insert(User &user);
    User getUserById(int id);
    bool updateUserInfo(const User &user);
    bool deleteUser(int id);
    void resetState();
};

#endif
