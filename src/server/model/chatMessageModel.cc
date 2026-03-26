#include "chatMessageModel.hpp"

#include <algorithm>
#include <memory>
#include <string>

#include "commonConnectionPool.hpp"

using namespace std;
using json = nlohmann::json;

namespace
{
long long insertMessage(const string &sessionType, int fromUserId, int toUserId,
                        int groupId, const string &content, const string &msgTime)
{
    shared_ptr<MySQL> sp = ConnectionPool::getConnectionPool()->getConnection();
    if (sp == nullptr)
    {
        return -1;
    }

    string escapedContent = sp->escapeString(content);
    string escapedTime = sp->escapeString(msgTime);
    string sql = "INSERT INTO chatmessage(session_type, from_userid, to_userid, group_id, content, msg_time) VALUES ('" +
                 sessionType + "', " + to_string(fromUserId) + ", ";
    if (toUserId >= 0)
    {
        sql += to_string(toUserId);
    }
    else
    {
        sql += "NULL";
    }
    sql += ", ";
    if (groupId >= 0)
    {
        sql += to_string(groupId);
    }
    else
    {
        sql += "NULL";
    }
    sql += ", '" + escapedContent + "', '" + escapedTime + "')";

    if (!sp->update(sql))
    {
        return -1;
    }
    return mysql_insert_id(sp->getMySQL());
}
}

ChatMessageModel::ChatMessageModel()
{
    initTable();
}

bool ChatMessageModel::initTable()
{
    shared_ptr<MySQL> sp = ConnectionPool::getConnectionPool()->getConnection();
    if (sp == nullptr)
    {
        return false;
    }

    string sql =
        "CREATE TABLE IF NOT EXISTS chatmessage ("
        "id BIGINT PRIMARY KEY AUTO_INCREMENT,"
        "session_type ENUM('single','group') NOT NULL,"
        "from_userid INT NOT NULL,"
        "to_userid INT DEFAULT NULL,"
        "group_id INT DEFAULT NULL,"
        "content TEXT NOT NULL,"
        "msg_time VARCHAR(32) NOT NULL,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "INDEX idx_chatmessage_user (to_userid, id),"
        "INDEX idx_chatmessage_group (group_id, id),"
        "INDEX idx_chatmessage_sender (from_userid, id)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    return sp->update(sql);
}

long long ChatMessageModel::insertSingleMessage(int fromUserId, int toUserId, const string &content, const string &msgTime)
{
    return insertMessage("single", fromUserId, toUserId, -1, content, msgTime);
}

long long ChatMessageModel::insertGroupMessage(int fromUserId, int groupId, const string &content, const string &msgTime)
{
    return insertMessage("group", fromUserId, -1, groupId, content, msgTime);
}

vector<json> ChatMessageModel::queryUserMessages(int userId, long long lastMessageId, size_t limit)
{
    vector<json> messages;
    shared_ptr<MySQL> sp = ConnectionPool::getConnectionPool()->getConnection();
    if (sp == nullptr)
    {
        return messages;
    }

    string orderBy = "ASC";
    long long lowerBound = lastMessageId;
    if (lastMessageId <= 0)
    {
        orderBy = "DESC";
        lowerBound = 0;
    }

    string sql =
        "SELECT id, session_type, from_userid, to_userid, group_id, content, msg_time "
        "FROM chatmessage cm "
        "WHERE cm.id > " + to_string(lowerBound) +
        " AND ((cm.session_type='single' AND (cm.from_userid=" + to_string(userId) + " OR cm.to_userid=" + to_string(userId) + "))"
        " OR (cm.session_type='group' AND EXISTS (SELECT 1 FROM groupuser gu WHERE gu.groupid = cm.group_id AND gu.userid = " + to_string(userId) + ")))"
        " ORDER BY cm.id " + orderBy + " LIMIT " + to_string(limit);

    MYSQL_RES *res = sp->query(sql);
    if (res == nullptr)
    {
        return messages;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr)
    {
        json item;
        item["messageid"] = row[0] ? atoll(row[0]) : 0;
        item["session_type"] = row[1] ? row[1] : "single";
        item["fromid"] = row[2] ? atoi(row[2]) : -1;
        item["toid"] = row[3] ? atoi(row[3]) : -1;
        item["groupid"] = row[4] ? atoi(row[4]) : -1;
        item["msg"] = row[5] ? row[5] : "";
        item["time"] = row[6] ? row[6] : "";
        messages.push_back(item);
    }
    mysql_free_result(res);

    if (lastMessageId <= 0)
    {
        reverse(messages.begin(), messages.end());
    }
    return messages;
}
