#include <muduo/base/Logging.h>
#include "offLineMsgModel.hpp"
#include "commonConnectionPool.hpp"

using namespace std;

// 存储离线消息的具体实现
void OffLineMsgModel::insert(int id, const string &msg)
{
    shared_ptr<MySQL> sp = ConnectionPool::getConnectionPool()->getConnection();
    if (sp == nullptr)
    {
        return;
    }
    string escapedMsg = sp->escapeString(msg);
    string sql = "INSERT INTO offlinemessage (userid, message) VALUES (" + to_string(id) + ", '" + escapedMsg + "')";
    sp->update(sql);
}

void OffLineMsgModel::remove(int userId)
{
    char sql[1024] = {0};
    sprintf(sql, "DELETE FROM offlinemessage WHERE userid = %d", userId);
    shared_ptr<MySQL> sp = ConnectionPool::getConnectionPool()->getConnection();
    sp->update(sql);
}

vector<string> OffLineMsgModel::get(int userId)
{
    char sql[1024] = {0};
    sprintf(sql, "SELECT message FROM offlinemessage WHERE userid = %d", userId);
    vector<string> vec;
    shared_ptr<MySQL> sp = ConnectionPool::getConnectionPool()->getConnection();
    MYSQL_RES *res = sp->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            vec.push_back(row[0]);
        }
        mysql_free_result(res);
    }
    return vec;
}
