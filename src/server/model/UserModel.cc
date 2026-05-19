#include "UserModel.hpp"
#include "commonConnectionPool.hpp"
#include "log.h"

using namespace std;

UserModel::UserModel() {}

bool UserModel::insert(User &user)
{
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    
    const string sql = "INSERT INTO user (name, password, state) VALUES (?, ?, ?)";
    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr) {
        LOG_ERROR << "Prepare statement failed";
        return false;
    }
    
    string name = user.getName();
    string pwd = user.getPassword();
    string state = user.getState();
    
    LOG_INFO << "Insert user: name=" << name 
             << ", password_len=" << pwd.length() 
             << ", state=" << state;
    
    MYSQL_BIND bind[3] = {0};
    unsigned long name_len = name.size();
    unsigned long pwd_len = pwd.size();
    unsigned long state_len = state.size();
    
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(name.c_str());
    bind[0].buffer_length = name_len;
    bind[0].length = &name_len;
    
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = const_cast<char*>(pwd.c_str());
    bind[1].buffer_length = pwd_len;
    bind[1].length = &pwd_len;
    
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = const_cast<char*>(state.c_str());
    bind[2].buffer_length = state_len;
    bind[2].length = &state_len;
    
    if (!sp->executeStmt(stmt, bind)) {
        LOG_ERROR << "executeStmt failed";
        sp->closeStmt(stmt);
        return false;
    }
    
    user.setId(mysql_stmt_insert_id(stmt));
    sp->closeStmt(stmt);
    return true;
}

User UserModel::getUserById(int id)
{
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    
    const string sql = "SELECT id, name, password, state FROM user WHERE id = ?";
    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr) {
        return User();
    }
    
    MYSQL_BIND param[1] = {0};
    param[0].buffer_type = MYSQL_TYPE_LONG;
    param[0].buffer = &id;
    
    if (!sp->executeStmt(stmt, param)) {
        sp->closeStmt(stmt);
        return User();
    }
    
    int result_id = 0;
    char result_name[256] = {0};
    char result_pwd[256] = {0};
    char result_state[32] = {0};
    unsigned long name_len = 0, pwd_len = 0, state_len = 0;
    
    MYSQL_BIND result[4] = {0};
    result[0].buffer_type = MYSQL_TYPE_LONG;
    result[0].buffer = &result_id;
    
    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = result_name;
    result[1].buffer_length = sizeof(result_name);
    result[1].length = &name_len;
    
    result[2].buffer_type = MYSQL_TYPE_STRING;
    result[2].buffer = result_pwd;
    result[2].buffer_length = sizeof(result_pwd);
    result[2].length = &pwd_len;
    
    result[3].buffer_type = MYSQL_TYPE_STRING;
    result[3].buffer = result_state;
    result[3].buffer_length = sizeof(result_state);
    result[3].length = &state_len;
    
    if (mysql_stmt_bind_result(stmt, result) != 0) {
        sp->closeStmt(stmt);
        return User();
    }
    
    User user(-1, "", "", "offline");
    if (mysql_stmt_fetch(stmt) == 0) {
        user.setId(result_id);
        user.setName(string(result_name, name_len));
        user.setPassword(string(result_pwd, pwd_len));
        user.setState(string(result_state, state_len));
    }
    
    sp->closeStmt(stmt);
    return user;
}

bool UserModel::updateUserInfo(const User &user)
{
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    
    const string sql = "UPDATE user SET name = ?, password = ?, state = ? WHERE id = ?";
    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr) {
        return false;
    }
    
    string name = user.getName();
    string pwd = user.getPassword();
    string state = user.getState();
    int id = user.getId();
    
    MYSQL_BIND bind[4] = {0};
    unsigned long name_len = name.size();
    unsigned long pwd_len = pwd.size();
    unsigned long state_len = state.size();
    
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(name.c_str());
    bind[0].buffer_length = name_len;
    bind[0].length = &name_len;
    
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = const_cast<char*>(pwd.c_str());
    bind[1].buffer_length = pwd_len;
    bind[1].length = &pwd_len;
    
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = const_cast<char*>(state.c_str());
    bind[2].buffer_length = state_len;
    bind[2].length = &state_len;
    
    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = &id;
    
    bool result = sp->executeStmt(stmt, bind);
    sp->closeStmt(stmt);
    return result;
}

void UserModel::resetState()
{
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    
    const string sql = "UPDATE user SET state = 'offline' WHERE state = 'online'";
    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr) {
        LOG_ERROR << "Prepare statement failed in resetState";
        return;
    }
    
    if (!sp->executeStmt(stmt, nullptr)) {
        LOG_ERROR << "executeStmt failed in resetState";
        sp->closeStmt(stmt);
        return;
    }
    
    sp->closeStmt(stmt);
}