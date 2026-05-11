#include "groupModel.hpp"
#include "log.h"
#include <vector>
#include <unordered_map>
#include <iostream>
#include "group.hpp"
#include "commonConnectionPool.hpp"
using namespace std;

bool GroupModel::createGroup(Group &group)
{
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    
    const string sql = "INSERT INTO allgroup(groupname, groupdesc) VALUES(?, ?)";
    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr) {
        return false;
    }
    
    string name = group.getName();
    string desc = group.getDesc();
    
    MYSQL_BIND bind[2] = {0};
    unsigned long name_len = name.size();
    unsigned long desc_len = desc.size();
    
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(name.c_str());
    bind[0].buffer_length = name_len;
    bind[0].length = &name_len;
    
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = const_cast<char*>(desc.c_str());
    bind[1].buffer_length = desc_len;
    bind[1].length = &desc_len;
    
    if (!sp->executeStmt(stmt, bind)) {
        sp->closeStmt(stmt);
        return false;
    }
    
    group.setId(mysql_stmt_insert_id(stmt));
    sp->closeStmt(stmt);
    return true;
}

bool GroupModel::addGroup(int userid, int groupid, string role)
{
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    
    // 先检查群组是否存在
    const string checkSql = "SELECT id FROM allgroup WHERE id = ?";
    MYSQL_STMT* checkStmt = sp->prepare(checkSql);
    if (checkStmt == nullptr) {
        return false;
    }
    
    MYSQL_BIND checkParam[1] = {0};
    checkParam[0].buffer_type = MYSQL_TYPE_LONG;
    checkParam[0].buffer = &groupid;
    
    if (!sp->executeStmt(checkStmt, checkParam)) {
        sp->closeStmt(checkStmt);
        return false;
    }
    
    int result_id = 0;
    MYSQL_BIND checkResult[1] = {0};
    checkResult[0].buffer_type = MYSQL_TYPE_LONG;
    checkResult[0].buffer = &result_id;
    
    if (mysql_stmt_bind_result(checkStmt, checkResult) != 0 ||
        mysql_stmt_fetch(checkStmt) != 0) {
        LOG_ERROR << "groupid: " << groupid << " 不存在";
        sp->closeStmt(checkStmt);
        return false;
    }
    sp->closeStmt(checkStmt);
    
    // 插入群成员
    const string insertSql = "INSERT INTO groupuser(groupid, userid, grouprole) VALUES(?, ?, ?)";
    MYSQL_STMT* insertStmt = sp->prepare(insertSql);
    if (insertStmt == nullptr) {
        return false;
    }
    
    MYSQL_BIND insertBind[3] = {0};
    unsigned long role_len = role.size();
    
    insertBind[0].buffer_type = MYSQL_TYPE_LONG;
    insertBind[0].buffer = &groupid;
    
    insertBind[1].buffer_type = MYSQL_TYPE_LONG;
    insertBind[1].buffer = &userid;
    
    insertBind[2].buffer_type = MYSQL_TYPE_STRING;
    insertBind[2].buffer = const_cast<char*>(role.c_str());
    insertBind[2].buffer_length = role_len;
    insertBind[2].length = &role_len;
    
    if (!sp->executeStmt(insertStmt, insertBind)) {
        LOG_ERROR << "userid: " << userid << " 加入 groupid: " << groupid << " 失败";
        sp->closeStmt(insertStmt);
        return false;
    }
    
    sp->closeStmt(insertStmt);
    return true;
}

// 查询用户所在群组并返回群组中所有用户信息
vector<Group> GroupModel::queryGroups(int userid)
{
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    
    const string sql = 
        "SELECT g.id, g.groupname, g.groupdesc, "
        "u.id, u.name, u.state, gu.grouprole "
        "FROM allgroup g "
        "INNER JOIN groupuser gu ON g.id = gu.groupid "
        "INNER JOIN user u ON gu.userid = u.id "
        "WHERE gu.groupid IN (SELECT groupid FROM groupuser WHERE userid = ?) "
        "ORDER BY g.id, u.id";

    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr)
    {
        LOG_ERROR << "userid: " << userid << " 所在群组查询失败";
        return {};
    }

    MYSQL_BIND bind[1] = {0};
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &userid;

    if (mysql_stmt_bind_param(stmt, bind) != 0)
    {
        sp->closeStmt(stmt);
        return {};
    }

    MYSQL_RES* res = sp->executeStmtQuery(stmt);
    if (!res)
    {
        LOG_ERROR << "userid: " << userid << " 所在群组查询失败";
        sp->closeStmt(stmt);
        return {};
    }

    MYSQL_BIND result[7] = {0};
    int groupid = 0;
    char groupname[256] = {0};
    unsigned long groupname_len = 0;
    char groupdesc[256] = {0};
    unsigned long groupdesc_len = 0;
    int user_id = 0;
    char username[256] = {0};
    unsigned long username_len = 0;
    char userstate[32] = {0};
    unsigned long userstate_len = 0;
    char grouprole[32] = {0};
    unsigned long grouprole_len = 0;

    result[0].buffer_type = MYSQL_TYPE_LONG;
    result[0].buffer = &groupid;

    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = groupname;
    result[1].buffer_length = sizeof(groupname);
    result[1].length = &groupname_len;

    result[2].buffer_type = MYSQL_TYPE_STRING;
    result[2].buffer = groupdesc;
    result[2].buffer_length = sizeof(groupdesc);
    result[2].length = &groupdesc_len;

    result[3].buffer_type = MYSQL_TYPE_LONG;
    result[3].buffer = &user_id;

    result[4].buffer_type = MYSQL_TYPE_STRING;
    result[4].buffer = username;
    result[4].buffer_length = sizeof(username);
    result[4].length = &username_len;

    result[5].buffer_type = MYSQL_TYPE_STRING;
    result[5].buffer = userstate;
    result[5].buffer_length = sizeof(userstate);
    result[5].length = &userstate_len;

    result[6].buffer_type = MYSQL_TYPE_STRING;
    result[6].buffer = grouprole;
    result[6].buffer_length = sizeof(grouprole);
    result[6].length = &grouprole_len;

    if (mysql_stmt_bind_result(stmt, result) != 0)
    {
        mysql_free_result(res);
        sp->closeStmt(stmt);
        return {};
    }

    vector<Group> groups;
    unordered_map<int, size_t> groupIndexMap;

    while (mysql_stmt_fetch(stmt) == 0)
    {
        if (groupIndexMap.find(groupid) == groupIndexMap.end())
        {
            Group group(groupid, string(groupname, groupname_len), string(groupdesc, groupdesc_len));
            groups.push_back(group);
            groupIndexMap[groupid] = groups.size() - 1;
        }

        groupUser user(user_id, string(username, username_len), string(userstate, userstate_len), string(grouprole, grouprole_len));
        groups[groupIndexMap[groupid]].getUsers().push_back(user);
    }

    mysql_free_result(res);
    sp->closeStmt(stmt);
    return groups;
}

// 查询群组中的所有用户
vector<groupUser> GroupModel::groupUsers(int groupid)
{
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    
    const string checkSql = "SELECT id FROM allgroup WHERE id = ?";
    MYSQL_STMT* checkStmt = sp->prepare(checkSql);
    if (checkStmt == nullptr)
    {
        LOG_ERROR << "groupid: " << groupid << " 不存在";
        return {};
    }

    MYSQL_BIND checkBind[1] = {0};
    checkBind[0].buffer_type = MYSQL_TYPE_LONG;
    checkBind[0].buffer = &groupid;

    if (mysql_stmt_bind_param(checkStmt, checkBind) != 0)
    {
        sp->closeStmt(checkStmt);
        return {};
    }

    int result_id = 0;
    MYSQL_BIND checkResult[1] = {0};
    checkResult[0].buffer_type = MYSQL_TYPE_LONG;
    checkResult[0].buffer = &result_id;

    if (mysql_stmt_bind_result(checkStmt, checkResult) != 0 ||
        mysql_stmt_fetch(checkStmt) != 0)
    {
        LOG_ERROR << "groupid: " << groupid << " 不存在";
        sp->closeStmt(checkStmt);
        return {};
    }
    sp->closeStmt(checkStmt);

    const string sql = 
        "SELECT a.id, a.name, a.state, b.grouprole "
        "FROM user a JOIN groupuser b ON a.id = b.userid WHERE b.groupid = ?";

    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr)
    {
        LOG_ERROR << "groupid: " << groupid << " 中的用户查询失败";
        return {};
    }

    MYSQL_BIND bind[1] = {0};
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &groupid;

    if (mysql_stmt_bind_param(stmt, bind) != 0)
    {
        sp->closeStmt(stmt);
        return {};
    }

    MYSQL_RES* res = sp->executeStmtQuery(stmt);
    if (!res)
    {
        LOG_ERROR << "groupid: " << groupid << " 中的用户查询失败";
        sp->closeStmt(stmt);
        return {};
    }

    MYSQL_BIND result[4] = {0};
    int user_id = 0;
    char username[256] = {0};
    unsigned long username_len = 0;
    char userstate[32] = {0};
    unsigned long userstate_len = 0;
    char grouprole[32] = {0};
    unsigned long grouprole_len = 0;

    result[0].buffer_type = MYSQL_TYPE_LONG;
    result[0].buffer = &user_id;

    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = username;
    result[1].buffer_length = sizeof(username);
    result[1].length = &username_len;

    result[2].buffer_type = MYSQL_TYPE_STRING;
    result[2].buffer = userstate;
    result[2].buffer_length = sizeof(userstate);
    result[2].length = &userstate_len;

    result[3].buffer_type = MYSQL_TYPE_STRING;
    result[3].buffer = grouprole;
    result[3].buffer_length = sizeof(grouprole);
    result[3].length = &grouprole_len;

    if (mysql_stmt_bind_result(stmt, result) != 0)
    {
        mysql_free_result(res);
        sp->closeStmt(stmt);
        return {};
    }

    vector<groupUser> groupusers;
    while (mysql_stmt_fetch(stmt) == 0)
    {
        groupUser gu(user_id, string(username, username_len), string(userstate, userstate_len), string(grouprole, grouprole_len));
        groupusers.push_back(gu);
    }

    mysql_free_result(res);
    sp->closeStmt(stmt);
    return groupusers;
}

bool GroupModel::deleteGroup(int groupid)
{
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    
    const string sql = "DELETE FROM allgroup WHERE id = ?";
    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr)
    {
        return false;
    }

    MYSQL_BIND bind[1] = {0};
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &groupid;

    if (mysql_stmt_bind_param(stmt, bind) != 0)
    {
        sp->closeStmt(stmt);
        return false;
    }

    bool result = sp->executeStmt(stmt, bind);
    sp->closeStmt(stmt);
    return result;
}
