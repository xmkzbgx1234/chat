#include "offLineMsgModel.hpp"
#include "commonConnectionPool.hpp"
#include "log.h"

using namespace std;

void OffLineMsgModel::insert(int id, const string &msg)
{
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    if (sp == nullptr) {
        LOG_ERROR << "Failed to get connection for insert offline message";
        return;
    }
    
    const string sql = "INSERT INTO offlinemessage (userid, message) VALUES (?, ?)";
    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr) {
        LOG_ERROR << "Failed to prepare statement for insert offline message";
        return;
    }
    
    MYSQL_BIND bind[2] = {0};
    unsigned long msg_len = msg.size();
    
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &id;
    
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = const_cast<char*>(msg.c_str());
    bind[1].buffer_length = msg_len;
    bind[1].length = &msg_len;
    
    if (!sp->executeStmt(stmt, bind)) {
        LOG_ERROR << "Failed to insert offline message for userid=" << id;
    }
    sp->closeStmt(stmt);
}

void OffLineMsgModel::remove(int userId)
{
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    if (sp == nullptr) {
        LOG_ERROR << "Failed to get connection for remove offline message";
        return;
    }
    
    const string sql = "DELETE FROM offlinemessage WHERE userid = ?";
    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr) {
        LOG_ERROR << "Failed to prepare statement for remove offline message";
        return;
    }
    
    MYSQL_BIND bind[1] = {0};
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &userId;
    
    if (!sp->executeStmt(stmt, bind)) {
        LOG_ERROR << "Failed to remove offline message for userid=" << userId;
    }
    sp->closeStmt(stmt);
}

vector<string> OffLineMsgModel::get(int userId)
{
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    
    const string sql = "SELECT message FROM offlinemessage WHERE userid = ?";
    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr) {
        return {};
    }
    
    MYSQL_BIND param[1] = {0};
    param[0].buffer_type = MYSQL_TYPE_LONG;
    param[0].buffer = &userId;
    
    if (!sp->executeStmt(stmt, param)) {
        sp->closeStmt(stmt);
        return {};
    }
    
    vector<string> vec;
    char result_msg[4096] = {0};
    unsigned long msg_len = 0;
    
    MYSQL_BIND result[1] = {0};
    result[0].buffer_type = MYSQL_TYPE_STRING;
    result[0].buffer = result_msg;
    result[0].buffer_length = sizeof(result_msg);
    result[0].length = &msg_len;
    
    if (mysql_stmt_bind_result(stmt, result) == 0) {
        while (mysql_stmt_fetch(stmt) == 0) {
            vec.push_back(string(result_msg, msg_len));
        }
    }
    
    sp->closeStmt(stmt);
    return vec;
}
