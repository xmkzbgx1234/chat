#ifndef CLIENT_COMMAND_DISPATCHER_HPP
#define CLIENT_COMMAND_DISPATCHER_HPP

#include <functional>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "clientSession.hpp"
#include "sqliteStorage.hpp"

class ClientCommandDispatcher {
public:
    using SendJson = std::function<bool(const nlohmann::json &)>;

    ClientCommandDispatcher(ClientSession &session, LocalChatStorage &storage, SendJson sendJson);

    void printHelp() const;
    bool execute(const std::string &commandLine);

private:
    bool handleHelp(const std::string &params);
    bool handleChat(const std::string &params);
    bool handleGroupChat(const std::string &params);
    bool handleAddFriend(const std::string &params);
    bool handleCreateGroup(const std::string &params);
    bool handleAddGroup(const std::string &params);
    bool handleHistory(const std::string &params);
    bool handleLogout(const std::string &params);

    static std::string currentTime();
    static void printDirectMessage(const nlohmann::json &js, const std::string &prefix);
    static void printGroupMessage(const nlohmann::json &js, const std::string &prefix);
    static std::string trim(const std::string &text);

    ClientSession &_session;
    LocalChatStorage &_storage;
    SendJson _sendJson;
    std::unordered_map<std::string, std::string> _commandDescMap;
    std::unordered_map<std::string, std::function<bool(const std::string &)>> _handlers;
};

#endif
