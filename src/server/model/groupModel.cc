#include "groupModel.hpp"
#include <vector>
#include <unordered_map>
#include <iostream>
#include <muduo/base/Logging.h>
#include "group.hpp"
#include "db.hpp"
using namespace std;

bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO allgroup(groupname, groupdesc) VALUES('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getMySQL()));
            // LOG_INFO << "groupid: " << group.getId() << " 创建成功";
            return true;
        }
    }
    return false;
}

bool GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "SELECT * FROM allgroup WHERE groupid=%d", groupid);
    MySQL mysql;
    if (mysql.connect())
    {
        if (!mysql.query(sql))
        {
            sprintf(sql, "INSERT INTO groupuser(groupid, userid, grouprole) VALUES(%d, %d, '%s')",
                    groupid, userid, role.c_str());
            if (mysql.update(sql))
            {
                // LOG_INFO << "userid: " << userid << " 加入 groupid: " << groupid << " 成功";
                return true;
            }
            else
            {
                LOG_ERROR << "userid: " << userid << " 加入 groupid: " << groupid << " 失败";
                return false;
            }
        }
        else
        {
            LOG_ERROR << "groupid: " << groupid << " 不存在";
            return false;
        }
    }
    else
    {
        LOG_ERROR << "数据库连接失败";
        return false;
    }
}

// 查询用户所在群组并返回群组中所有用户信息
vector<Group> GroupModel::queryGroups(int userid)
{
    MySQL mysql;
    if (!mysql.connect())
    {
        LOG_ERROR << "userid: " << userid << " 数据库连接失败";
        return {};
    }

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

    MYSQL_RES *res = mysql.query(sql);
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
        for (int i = 0; i < mysql_num_fields(res); i++)
        {
            cout << (row[i] ? row[i] : "NULL") << " ";
        }
        cout << endl;

        // 2. 解析字段（补充空值处理）
        int groupid = atoi(row[0] ? row[0] : "0");
        const char *groupname = row[1] ? row[1] : "";
        const char *groupdesc = row[2] ? row[2] : "";

        int userid_ = atoi(row[3] ? row[3] : "0");
        const char *username = row[4] ? row[4] : "";
        const char *userstate = row[5] ? row[5] : "offline"; // 在线状态默认值
        const char *grouprole = row[6] ? row[6] : "normal";   // 群角色默认值

        // 3. 构建群组（未存在则新增）
        if (groupIndexMap.find(groupid) == groupIndexMap.end())
        {
            Group group(groupid, groupname, groupdesc);
            groups.push_back(group);
            groupIndexMap[groupid] = groups.size() - 1;
        }

        // ========== 核心修正2：groupUser构造参数传对（role=grouprole，state=userstate） ==========
        // 假设groupUser构造函数：groupUser(int id, string name, string state, string role)
        // 传参顺序：id, name, state(userstate), role(grouprole)
        groupUser user(userid_, username, grouprole, userstate);
        
        // 若groupUser构造函数是：groupUser(int id, string name, string role, string state)
        // 则改为：groupUser user(userid_, username, grouprole, userstate);

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
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res)
        {
            sprintf(sql, "SELECT a.id, a.name, a.state, b.grouprole FROM user a \
        JOIN groupuser b ON a.id = b.userid WHERE b.groupid=%d",
                    groupid);
            res = mysql.query(sql);
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
    }
    return vector<groupUser>();
}