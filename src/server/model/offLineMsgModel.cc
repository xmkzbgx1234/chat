#include <muduo/base/Logging.h>
#include "offLineMsgModel.hpp"
#include "db.hpp"

using namespace std;
 
// 存储离线消息的具体实现
void OffLineMessageModel::insert(int id, const string &msg)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO offlinemessage (userid, message) VALUES (%d, '%s')",
             id, msg.c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
        return;
    }
}

void OffLineMessageModel::remove(int userId)
{
    char sql[1024] = {0};
    sprintf(sql, "DELETE FROM offlinemessage WHERE userid = %d", userId);
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

vector<string> OffLineMessageModel::get(int userId)
{
    char sql[1024] = {0};
    sprintf(sql, "SELECT message FROM offlinemessage WHERE userid = %d", userId);
    vector<string> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
        }
        // else {
        //     LOG_INFO << "没有离线消息";
        // }
    }
    return vec;
}