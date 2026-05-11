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
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
    if (sp == nullptr) {
        return -1;
    }

    const string sql = "INSERT INTO chatmessage(session_type, from_userid, to_userid, group_id, content, msg_time) VALUES (?, ?, ?, ?, ?, ?)";
    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr) {
        return -1;
    }
    
    MYSQL_BIND bind[6] = {0};
    unsigned long session_len = sessionType.size();
    unsigned long content_len = content.size();
    unsigned long time_len = msgTime.size();
    bool toUserId_null = (toUserId < 0) ? 1 : 0;
    bool groupId_null = (groupId < 0) ? 1 : 0;
    
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(sessionType.c_str());
    bind[0].buffer_length = session_len;
    bind[0].length = &session_len;
    
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &fromUserId;
    
    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &toUserId;
    bind[2].is_null = &toUserId_null;
    
    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = &groupId;
    bind[3].is_null = &groupId_null;
    
    bind[4].buffer_type = MYSQL_TYPE_STRING;
    bind[4].buffer = const_cast<char*>(content.c_str());
    bind[4].buffer_length = content_len;
    bind[4].length = &content_len;
    
    bind[5].buffer_type = MYSQL_TYPE_STRING;
    bind[5].buffer = const_cast<char*>(msgTime.c_str());
    bind[5].buffer_length = time_len;
    bind[5].length = &time_len;
    
    if (!sp->executeStmt(stmt, bind)) {
        sp->closeStmt(stmt);
        return -1;
    }
    
    long long id = mysql_stmt_insert_id(stmt);
    sp->closeStmt(stmt);
    return id;
}
}

ChatMessageModel::ChatMessageModel()
{
    initTable();
}

bool ChatMessageModel::initTable()
{
    ConnectionPool::ConnectionPtr sp = ConnectionPool::getConnectionPool()->getConnection();
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
    auto sp = ConnectionPool::getConnectionPool()->getConnection();
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
        "WHERE cm.id > ? AND ((cm.session_type='single' AND (cm.from_userid=? OR cm.to_userid=?))"
        " OR (cm.session_type='group' AND EXISTS (SELECT 1 FROM groupuser gu WHERE gu.groupid = cm.group_id AND gu.userid = ?)))"
        " ORDER BY cm.id " + orderBy + " LIMIT ?";

    MYSQL_STMT* stmt = sp->prepare(sql);
    if (stmt == nullptr)
    {
        return messages;
    }

    unsigned long long boundLower = static_cast<unsigned long long>(lowerBound);
    unsigned int boundUserId = static_cast<unsigned int>(userId);
    unsigned long long boundLimit = static_cast<unsigned long long>(limit);

    MYSQL_BIND bind[5] = {0};
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = &boundLower;
    bind[0].is_unsigned = true;

    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &boundUserId;
    bind[1].is_unsigned = true;

    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &boundUserId;
    bind[2].is_unsigned = true;

    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = &boundUserId;
    bind[3].is_unsigned = true;

    bind[4].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[4].buffer = &boundLimit;
    bind[4].is_unsigned = true;

    if (mysql_stmt_bind_param(stmt, bind) != 0)
    {
        sp->closeStmt(stmt);
        return messages;
    }

    MYSQL_RES* res = sp->executeStmtQuery(stmt);
    if (res == nullptr)
    {
        sp->closeStmt(stmt);
        return messages;
    }

    MYSQL_BIND result[7] = {0};
    long long id = 0;
    char session_type[16] = {0};
    unsigned long session_len = 0;
    int from_userid = 0;
    int to_userid = 0;
    my_bool to_userid_null = 0;
    int group_id = 0;
    my_bool group_id_null = 0;
    char content[65536] = {0};
    unsigned long content_len = 0;
    char msg_time[64] = {0};
    unsigned long time_len = 0;

    result[0].buffer_type = MYSQL_TYPE_LONGLONG;
    result[0].buffer = &id;

    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = session_type;
    result[1].buffer_length = sizeof(session_type);
    result[1].length = &session_len;

    result[2].buffer_type = MYSQL_TYPE_LONG;
    result[2].buffer = &from_userid;

    result[3].buffer_type = MYSQL_TYPE_LONG;
    result[3].buffer = &to_userid;
    result[3].is_null = &to_userid_null;

    result[4].buffer_type = MYSQL_TYPE_LONG;
    result[4].buffer = &group_id;
    result[4].is_null = &group_id_null;

    result[5].buffer_type = MYSQL_TYPE_STRING;
    result[5].buffer = content;
    result[5].buffer_length = sizeof(content);
    result[5].length = &content_len;

    result[6].buffer_type = MYSQL_TYPE_STRING;
    result[6].buffer = msg_time;
    result[6].buffer_length = sizeof(msg_time);
    result[6].length = &time_len;

    if (mysql_stmt_bind_result(stmt, result) != 0)
    {
        mysql_free_result(res);
        sp->closeStmt(stmt);
        return messages;
    }

    while (mysql_stmt_fetch(stmt) == 0)
    {
        json item;
        item["messageid"] = id;
        item["session_type"] = string(session_type, session_len);
        item["fromid"] = from_userid;
        item["toid"] = to_userid_null ? -1 : to_userid;
        item["groupid"] = group_id_null ? -1 : group_id;
        item["msg"] = string(content, content_len);
        item["time"] = string(msg_time, time_len);
        messages.push_back(item);
    }

    mysql_free_result(res);
    sp->closeStmt(stmt);

    if (lastMessageId <= 0)
    {
        reverse(messages.begin(), messages.end());
    }
    return messages;
}
