#include "UserModel.hpp"
#include "commonConnectionPool.hpp"
#include <muduo/base/Logging.h>

using namespace std;

UserModel::UserModel() {}

bool UserModel::insert(User &user)
{
    // 使用db对象进行数据库操作
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO user (name, password, state) VALUES ('%s', '%s', '%s')",
            user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());
    shared_ptr<MySQL> sp = ConnectionPool::getConnectionPool()->getConnection();
    if (sp->update(sql))
    {
        // 获取插入成功的用户数据生成的主键id
        user.setId(mysql_insert_id(sp->getMySQL()));
        return true;
    }
    else
    {
        return false;
    }
}

User UserModel::getUserById(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "SELECT id, name, password, state FROM user WHERE id = %d", id);
    shared_ptr<MySQL> sp = ConnectionPool::getConnectionPool()->getConnection();

    MYSQL_RES *res = sp->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row = mysql_fetch_row(res);
        User user(-1, "", "", "offline");
        if (row != nullptr)
        {
            user.setId(atoi(row[0]));
            user.setName(row[1] ? row[1] : "");
            user.setPassword(row[2] ? row[2] : "");
            user.setState(row[3] ? row[3] : "");
        }
        mysql_free_result(res); // 释放资源
        return user;
    }
    else
    {
        return User();
    }
}

bool UserModel::updateUserInfo(const User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "UPDATE user SET name = '%s', password = '%s', state = '%s' WHERE id = %d",
            user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str(), user.getId());
    shared_ptr<MySQL> sp = ConnectionPool::getConnectionPool()->getConnection();

    if (sp->update(sql))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void UserModel::resetState()
{
    // 把所有在线用户的状态设置为离线
    char sql[1024] = "UPDATE user SET state = 'offline' WHERE state = 'online'";
    shared_ptr<MySQL> sp = ConnectionPool::getConnectionPool()->getConnection();
    sp->update(sql);
}
