#include "groupModel.hpp"
#include <vector>
#include <unordered_map>
#include <iostream>
#include <muduo/base/Logging.h>
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
    ConnectionPool::ConnectionPtr sp = ConnectionPool::getConnectionPool()->getConnection();
    // 单次 SQL 查询所有数据
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT g.id, g.groupname, g.groupdesc, "
             "u.id, u.name, u.state, gu.grouprole "
             "FROM allgroup g "
             "INNER JOIN groupuser gu ON g.id = gu.groupid "
             "INNER JOIN user u ON gu.userid = u.id "
             "WHERE gu.groupid IN (SELECT groupid FROM groupuser WHERE userid = %d) "
             "ORDER BY g.id, u.id",
             userid);

    MYSQL_RES *res = sp->query(sql);
    if (!res)
    {
        LOG_ERROR << "userid: " << userid << " 所在群组查询失败";
        return {};
    }

    // LOG_INFO << "userid: " << userid << " 所在群组查询成功";

    vector<Group> groups;
    unordered_map<int, size_t> groupIndexMap;

    // ========== 核心修正1：合并「打印SQL结果」和「构建groups」逻辑 ==========
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
    {
        // 1. 先打印SQL原始结果（调试用）
        // 2. 解析字段（补充空值处理）
        int groupid = atoi(row[0] ? row[0] : "0");
        const char *groupname = row[1] ? row[1] : "";
        const char *groupdesc = row[2] ? row[2] : "";

        int userid_ = atoi(row[3] ? row[3] : "0");
        const char *username = row[4] ? row[4] : "";
        const char *userstate = row[5] ? row[5] : "offline"; // 在线状态默认值
        const char *grouprole = row[6] ? row[6] : "normal";  // 群角色默认值

        // 3. 构建群组（未存在则新增）
        if (groupIndexMap.find(groupid) == groupIndexMap.end())
        {
            Group group(groupid, groupname, groupdesc);
            groups.push_back(group);
            groupIndexMap[groupid] = groups.size() - 1;
        }

        groupUser user(userid_, username, userstate, grouprole);

        // 4. 添加成员到群组
        Group &targetGroup = groups[groupIndexMap[groupid]];
        targetGroup.getUsers().push_back(user);
    }

    // 打印构建后的groups（验证）
    // cout << "===== 构建后的groups =====" << endl;
    // for (Group &group : groups)
    // {
    //     cout << "groupid: " << group.getId() << " name: " << group.getName() << " desc: " << group.getDesc() << endl;
    //     for (const groupUser &user : group.getUsers())
    //     {
    //         cout << "userid: " << user.getId() << " name: " << user.getName()
    //              << " state: " << user.getState()  // 应输出online/offline
    //              << " role: " << user.getRole()    // 应输出creator/normal
    //              << endl;
    //     }
    // }

    mysql_free_result(res);
    return groups;
}

// 查询群组中的所有用户
vector<groupUser> GroupModel::groupUsers(int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "SELECT * FROM allgroup WHERE id=%d", groupid);
    ConnectionPool::ConnectionPtr sp = ConnectionPool::getConnectionPool()->getConnection();

    MYSQL_RES *res = sp->query(sql);
    if (res)
    {
        sprintf(sql, "SELECT a.id, a.name, a.state, b.grouprole FROM user a \
        JOIN groupuser b ON a.id = b.userid WHERE b.groupid=%d",
                groupid);
        res = sp->query(sql);
        if (res)
        {
            // LOG_INFO << "groupid: " << groupid << " 中的用户查询成功";
            vector<groupUser> groupusers;
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)))
            {
                groupUser groupuser(atoi(row[0]), row[1], row[2], row[3]);
                groupusers.push_back(groupuser);
            }
            mysql_free_result(res);
            return groupusers;
        }
        else
        {
            LOG_ERROR << "groupid: " << groupid << " 中的用户查询失败";
            return vector<groupUser>();
        }
    }
    else
    {
        LOG_ERROR << "groupid: " << groupid << " 不存在";
        return vector<groupUser>();
    }
    mysql_free_result(res);
    return vector<groupUser>();
}
