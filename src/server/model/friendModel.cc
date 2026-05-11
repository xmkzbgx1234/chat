#include "friendModel.hpp"
#include "commonConnectionPool.hpp"
#include <muduo/base/Logging.h>
using namespace std;

FriendModel::FriendModel()
{
}

// 添加好友
void FriendModel::insert(int userid, int friendid)
{
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    
    const string sql = "INSERT INTO friend VALUES (?, ?)";
    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr) {
        return;
    }
    
    MYSQL_BIND bind[2] = {0};
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &userid;
    
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &friendid;
    
    sp->executeStmt(stmt, bind);
    sp->closeStmt(stmt);
}

vector<User> FriendModel::query(int userid)
{
    const string sql = 
        "SELECT DISTINCT u.id, u.name, u.state FROM user u "
        "WHERE u.id IN ("
        "   SELECT CASE WHEN b.userid = ? THEN b.friendid ELSE b.userid END "
        "   FROM friend b "
        "   WHERE b.userid = ? OR b.friendid = ?"
        ") AND u.id != ?";
    
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr) {
        return {};
    }
    
    MYSQL_BIND bind[4] = {0};
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &userid;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &userid;
    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &userid;
    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = &userid;
    
    if (!sp->executeStmt(stmt, bind)) {
        sp->closeStmt(stmt);
        return {};
    }
    
    MYSQL_RES* result = mysql_stmt_result_metadata(stmt);
    if (result == nullptr) {
        sp->closeStmt(stmt);
        return {};
    }
    
    vector<User> friends;
    int result_id = 0;
    char result_name[256] = {0};
    char result_state[32] = {0};
    unsigned long name_len = 0, state_len = 0;
    
    MYSQL_BIND result_bind[3] = {0};
    result_bind[0].buffer_type = MYSQL_TYPE_LONG;
    result_bind[0].buffer = &result_id;
    
    result_bind[1].buffer_type = MYSQL_TYPE_STRING;
    result_bind[1].buffer = result_name;
    result_bind[1].buffer_length = sizeof(result_name);
    result_bind[1].length = &name_len;
    
    result_bind[2].buffer_type = MYSQL_TYPE_STRING;
    result_bind[2].buffer = result_state;
    result_bind[2].buffer_length = sizeof(result_state);
    result_bind[2].length = &state_len;
    
    if (mysql_stmt_bind_result(stmt, result_bind) == 0) {
        while (mysql_stmt_fetch(stmt) == 0) {
            User user;
            user.setId(result_id);
            user.setName(string(result_name, name_len));
            user.setState(string(result_state, state_len));
            friends.push_back(user);
        }
    }
    
    mysql_free_result(result);
    sp->closeStmt(stmt);
    return friends;
}
