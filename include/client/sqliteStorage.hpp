#ifndef SQLITE_STORAGE_HPP
#define SQLITE_STORAGE_HPP

#include <mutex>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <sqlite3.h>

class LocalChatStorage
{
public:
    static LocalChatStorage &instance();

    bool init(const std::string &dbPath = "chat_client.db");
    long long getLastSyncCursor(int userId);
    bool updateSyncCursor(int userId, long long cursor);
    bool saveMessage(int ownerUserId, const nlohmann::json &message);
    bool saveMessages(int ownerUserId, const std::vector<nlohmann::json> &messages);
    std::vector<nlohmann::json> querySessionMessages(int ownerUserId, const std::string &sessionType, int peerId, int limit = 20);

private:
    LocalChatStorage();
    ~LocalChatStorage();
    LocalChatStorage(const LocalChatStorage &) = delete;
    LocalChatStorage &operator=(const LocalChatStorage &) = delete;

    bool ensureTables();

    sqlite3 *_db;
    std::mutex _mutex;
};

#endif
