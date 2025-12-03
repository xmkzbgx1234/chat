#include "UserModel.hpp"
#include "db/db.hpp"
#include <muduo/base/Logging.h>

UserModel::UserModel() {}

bool UserModel::insert(User &user)
{
    // 使用db对象进行数据库操作
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "INSERT INTO user (name, password, state) VALUES ('%s', '%s', '%s')",
             user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());
    MySQL mysql;
    if(mysql.connect() == false){
        LOG_ERROR << "connect mysql failed!";
        return false;
    }
    else {
        if(mysql.update(sql)){
            // 获取插入成功的用户数据生成的主键id
            user.setId(mysql_insert_id(mysql.getMySQL()));
            return true;
        }
        else {
            return false;
        }
    }
}

User UserModel::getUserById(int id)
{
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "SELECT id, name, password, state FROM user WHERE id = %d", id);
    MySQL mysql;
    if(mysql.connect() == false){
        LOG_ERROR << "connect mysql failed!";
        return User();
    }
    else {
        MYSQL_RES *res = mysql.query(sql); 
        if(res != nullptr){
            MYSQL_ROW row = mysql_fetch_row(res);
            User user(-1, "", "", "offline");
            if(row != nullptr){
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
            }
            mysql_free_result(res); // 释放资源
            return user;
        }
        else {
            return User();
        }
    }
}

bool UserModel::updateUserInfo(const User &user)
{
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "UPDATE user SET name = '%s', password = '%s', state = '%s' WHERE id = %d",
             user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str(), user.getId());
    MySQL mysql;
    if(mysql.connect() == false){
        LOG_ERROR << "connect mysql failed!";
        return false;
    }
    else {
        if(mysql.update(sql)){
            return true;
        }
        else {
            return false;
        }
    }
}