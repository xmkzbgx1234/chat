#ifndef CLIENT_APP_HPP
#define CLIENT_APP_HPP

#include <string>

#include <nlohmann/json.hpp>

#include "clientCommandDispatcher.hpp"
#include "clientSession.hpp"
#include "sqliteStorage.hpp"

class ClientApp {
public:
    ClientApp();
    int run(int argc, char *argv[]);

private:
    bool initStorage();
    bool connectServer(const std::string &ip, int port);
    void closeConnection();
    bool recvExact(void *buffer, std::size_t size);
    bool recvJson(nlohmann::json &js);
    void startupMenu();
    bool login();
    bool registerUser();
    void mainMenu();
    void readLoop();
    void handleIncoming(const nlohmann::json &js);
    void syncHistoryFromLogin(const nlohmann::json &recvjs);
    void storeMessageIfPossible(const nlohmann::json &message);

    static void printDirectMessage(const nlohmann::json &js, const std::string &prefix);
    static void printGroupMessage(const nlohmann::json &js, const std::string &prefix);

    bool sendJson(const nlohmann::json &js);

    int _sockfd;
    ClientSession _session;
    LocalChatStorage &_storage;
    ClientCommandDispatcher _dispatcher;
};

#endif
