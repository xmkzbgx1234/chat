#include "friendModel.hpp"
#include "db/db.hpp"
#include <muduo/base/Logging.h>
using namespace std;

FriendModel::FriendModel()
{
}

// 添加好友
void FriendModel::insert(int userid, int friendid){
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO friend VALUES (%d, %d)", userid, friendid);
    MySQL mysql;
    if(mysql.connect()){
        // LOG_INFO << "mysql.update";
        mysql.update(sql);
    }
}

vector<User> FriendModel::query(int userid){
    char sql[1024] = {0};
    // 联合查询
    snprintf(sql, sizeof(sql),
        "SELECT DISTINCT u.id, u.name, u.state FROM user u "
        "WHERE u.id IN ("
        "   SELECT CASE WHEN b.userid = %d THEN b.friendid ELSE b.userid END "
        "   FROM friend b "
        "   WHERE b.userid = %d OR b.friendid = %d"
        ") AND u.id != %d",
        userid, userid, userid, userid);
    vector<User> frends;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                frends.push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return frends;
}