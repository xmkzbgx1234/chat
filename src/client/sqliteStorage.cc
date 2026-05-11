#include "sqliteStorage.hpp"

#include "log.h"

#include <algorithm>

using namespace std;
using json = nlohmann::json;

LocalChatStorage::LocalChatStorage() : _db(nullptr)
{
}

LocalChatStorage::~LocalChatStorage()
{
    if (_db != nullptr)
    {
        sqlite3_close(_db);
        _db = nullptr;
    }
}

// 单例
LocalChatStorage &LocalChatStorage::instance()
{
    static LocalChatStorage storage;
    return storage;
}


// 初始化
bool LocalChatStorage::init(const string &dbPath)
{
    lock_guard<mutex> lock(_mutex);
    if (_db != nullptr)
    {
        return true;
    }

    if (sqlite3_open(dbPath.c_str(), &_db) != SQLITE_OK)
    {
        LOG_ERROR << "sqlite open failed: " << sqlite3_errmsg(_db);
        return false;
    }
    return ensureTables();
}

bool LocalChatStorage::ensureTables()
{
    const char *messageSql =
        "CREATE TABLE IF NOT EXISTS local_message("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "owner_userid INTEGER NOT NULL,"
        "server_message_id INTEGER NOT NULL,"
        "session_type TEXT NOT NULL,"
        "from_userid INTEGER NOT NULL,"
        "to_userid INTEGER,"
        "group_id INTEGER,"
        "content TEXT NOT NULL,"
        "msg_time TEXT NOT NULL,"
        "UNIQUE(owner_userid, server_message_id)"
        ");";
    const char *cursorSql =
        "CREATE TABLE IF NOT EXISTS sync_cursor("
        "owner_userid INTEGER PRIMARY KEY,"
        "last_server_message_id INTEGER NOT NULL DEFAULT 0"
        ");";

    char *errMsg = nullptr;
    if (sqlite3_exec(_db, messageSql, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        LOG_ERROR << "create local_message failed: " << errMsg;
        sqlite3_free(errMsg);
        return false;
    }
    if (sqlite3_exec(_db, cursorSql, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        LOG_ERROR << "create sync_cursor failed: " << errMsg;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

// 增量同步服务端消息记录的游标
long long LocalChatStorage::getLastSyncCursor(int userId)
{
    lock_guard<mutex> lock(_mutex);
    if (_db == nullptr)
    {
        return 0;
    }

    sqlite3_stmt *stmt = nullptr;
    long long cursor = 0;
    const char *sql = "SELECT last_server_message_id FROM sync_cursor WHERE owner_userid = ?";
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return 0;
    }
    sqlite3_bind_int(stmt, 1, userId);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        cursor = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return cursor;
}

bool LocalChatStorage::updateSyncCursor(int userId, long long cursor)
{
    lock_guard<mutex> lock(_mutex);
    if (_db == nullptr)
    {
        return false;
    }

    sqlite3_stmt *stmt = nullptr;
    const char *sql =
        "INSERT INTO sync_cursor(owner_userid, last_server_message_id) VALUES(?, ?) "
        "ON CONFLICT(owner_userid) DO UPDATE SET last_server_message_id=excluded.last_server_message_id";
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int64(stmt, 2, cursor);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool LocalChatStorage::saveMessage(int ownerUserId, const json &message)
{
    lock_guard<mutex> lock(_mutex);
    if (_db == nullptr || !message.contains("messageid"))
    {
        return false;
    }

    sqlite3_stmt *stmt = nullptr;
    const char *sql =
        "INSERT OR IGNORE INTO local_message("
        "owner_userid, server_message_id, session_type, from_userid, to_userid, group_id, content, msg_time"
        ") VALUES(?, ?, ?, ?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, ownerUserId);
    sqlite3_bind_int64(stmt, 2, message.value("messageid", 0LL));
    string sessionType = message.value("session_type", "single");
    sqlite3_bind_text(stmt, 3, sessionType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, message.value("fromid", message.value("id", -1)));
    if (message.contains("toid") && !message["toid"].is_null())
    {
        sqlite3_bind_int(stmt, 5, message.value("toid", -1));
    }
    else
    {
        sqlite3_bind_null(stmt, 5);
    }
    if (message.contains("groupid") && !message["groupid"].is_null() && message.value("groupid", -1) >= 0)
    {
        sqlite3_bind_int(stmt, 6, message.value("groupid", -1));
    }
    else
    {
        sqlite3_bind_null(stmt, 6);
    }
    string content = message.value("msg", "");
    string msgTime = message.value("time", "");
    sqlite3_bind_text(stmt, 7, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, msgTime.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool LocalChatStorage::saveMessages(int ownerUserId, const vector<json> &messages)
{
    long long maxCursor = getLastSyncCursor(ownerUserId);
    for (const json &message : messages)
    {
        saveMessage(ownerUserId, message);
        maxCursor = max(maxCursor, message.value("messageid", 0LL));
    }
    return updateSyncCursor(ownerUserId, maxCursor);
}

vector<json> LocalChatStorage::querySessionMessages(int ownerUserId, const string &sessionType, int peerId, int limit)
{
    lock_guard<mutex> lock(_mutex);
    vector<json> messages;
    if (_db == nullptr)
    {
        return messages;
    }

    sqlite3_stmt *stmt = nullptr;
    string sql;
    if (sessionType == "single")
    {
        sql =
            "SELECT server_message_id, from_userid, to_userid, content, msg_time FROM local_message "
            "WHERE owner_userid = ? AND session_type = 'single' "
            "AND ((from_userid = ? AND to_userid = ?) OR (from_userid = ? AND to_userid = ?)) "
            "ORDER BY server_message_id DESC LIMIT ?";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            return messages;
        }
        sqlite3_bind_int(stmt, 1, ownerUserId);
        sqlite3_bind_int(stmt, 2, ownerUserId);
        sqlite3_bind_int(stmt, 3, peerId);
        sqlite3_bind_int(stmt, 4, peerId);
        sqlite3_bind_int(stmt, 5, ownerUserId);
        sqlite3_bind_int(stmt, 6, limit);
    }
    else
    {
        sql =
            "SELECT server_message_id, from_userid, group_id, content, msg_time FROM local_message "
            "WHERE owner_userid = ? AND session_type = 'group' AND group_id = ? "
            "ORDER BY server_message_id DESC LIMIT ?";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            return messages;
        }
        sqlite3_bind_int(stmt, 1, ownerUserId);
        sqlite3_bind_int(stmt, 2, peerId);
        sqlite3_bind_int(stmt, 3, limit);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        json item;
        item["messageid"] = sqlite3_column_int64(stmt, 0);
        item["fromid"] = sqlite3_column_int(stmt, 1);
        if (sessionType == "single")
        {
            item["toid"] = sqlite3_column_int(stmt, 2);
            item["msg"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            item["time"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        }
        else
        {
            item["groupid"] = sqlite3_column_int(stmt, 2);
            item["msg"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            item["time"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        }
        messages.push_back(item);
    }
    sqlite3_finalize(stmt);
    std::reverse(messages.begin(), messages.end());
    return messages;
}
