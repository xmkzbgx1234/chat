#include "clientApp.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <limits>
#include <thread>
#include <vector>

#include "log.h"
#include "public.hpp"

using namespace std;
using json = nlohmann::json;

ClientApp::ClientApp()
    : _sockfd(-1),
      _storage(LocalChatStorage::instance()),
      _dispatcher(_session, _storage, [this](const json &js) { return sendJson(js); })
{}

int ClientApp::run(int argc, char *argv[])
{
    if (!initStorage())
    {
        return -1;
    }
    if (argc < 3)
    {
        LOG_ERROR << "Invalid command format, usage: ./chat_client <ip> <port>";
        return -1;
    }

    string ip = argv[1];
    int port = atoi(argv[2]);
    if (!connectServer(ip, port))
    {
        return -1;
    }

    startupMenu();
    closeConnection();
    return 0;
}

bool ClientApp::initStorage()
{
    return _storage.init();
}

bool ClientApp::connectServer(const string &ip, int port)
{
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockfd < 0)
    {
        LOG_ERROR << "Create client socket failed";
        return false;
    }

    sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serveraddr.sin_addr);
    if (connect(_sockfd, (sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        LOG_ERROR << "Connect server failed, ip=" << ip << ", port=" << port;
        closeConnection();
        return false;
    }
    return true;
}

void ClientApp::closeConnection()
{
    if (_sockfd >= 0)
    {
        close(_sockfd);
        _sockfd = -1;
    }
}

void ClientApp::startupMenu()
{
    for (;;)
    {
        cout << "1. 登录" << endl;
        cout << "2. 注册" << endl;
        cout << "3. 退出" << endl;
        cout << "请输入您的选择：" << endl;

        int choice = 0;
        if (!(cin >> choice))
        {
            if (cin.eof())
            {
                return;
            }
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "输入无效，请输入数字！" << endl;
            continue;
        }

        switch (choice)
        {
        case 1:
            if (login())
            {
                mainMenu();
            }
            break;
        case 2:
            registerUser();
            break;
        case 3:
            return;
        default:
            cout << "输入错误，请重新输入！" << endl;
            break;
        }
    }
}

bool ClientApp::login()
{
    int id = 0;
    char password[50] = {0};
    cout << "请输入账号：" << endl;
    cin >> id;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "请输入密码：" << endl;
    cin.getline(password, sizeof(password));

    json request;
    request["msgid"] = LOGIN_MSG;
    request["id"] = id;
    request["password"] = password;
    request["sync_cursor"] = _storage.getLastSyncCursor(id);
    if (!sendJson(request))
    {
        LOG_ERROR << "Send login request failed, userId=" << id;
        return false;
    }

    char buffer[8192] = {0};
    int len = recv(_sockfd, buffer, sizeof(buffer), 0);
    if (len <= 0)
    {
        LOG_ERROR << "Receive login response failed, userId=" << id;
        return false;
    }

    json recvjs = json::parse(buffer);
    if (recvjs["msgid"].get<int>() != LOGIN_MSG_ACK || recvjs["errno"].get<int>() != 0)
    {
        LOG_WARN << "Login failed, userId=" << id << ", reason="
                 << recvjs.value("errmsg", string("未知错误"));
        return false;
    }

    LOG_INFO << "Login succeeded, userId=" << recvjs.value("id", id);
    _session.applyLogin(recvjs, id);
    syncHistoryFromLogin(recvjs);
    _session.showCurrentUserInfo();

    if (recvjs.contains("offlinemsg") && recvjs["offlinemsg"].is_array())
    {
        for (const json &offlineMessageJson : recvjs["offlinemsg"])
        {
            json offlineMessage = json::parse(offlineMessageJson.get<string>());
            if (offlineMessage.value("session_type", string("single")) == "group")
            {
                printGroupMessage(offlineMessage, "离线群消息：");
            }
            else
            {
                printDirectMessage(offlineMessage, "离线消息：");
            }
            storeMessageIfPossible(offlineMessage);
        }
    }

    thread readTask(&ClientApp::readLoop, this);
    readTask.detach();
    return true;
}

bool ClientApp::registerUser()
{
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    char name[50] = {0};
    char password[50] = {0};
    cout << "请输入用户名：" << endl;
    cin.getline(name, sizeof(name));
    cout << "请输入密码：" << endl;
    cin.getline(password, sizeof(password));

    json request;
    request["msgid"] = REG_MSG;
    request["name"] = name;
    request["password"] = password;
    if (!sendJson(request))
    {
        LOG_ERROR << "Send register request failed";
        return false;
    }

    char buffer[1024] = {0};
    int len = recv(_sockfd, buffer, sizeof(buffer), 0);
    if (len <= 0)
    {
        LOG_ERROR << "Receive register response failed";
        return false;
    }

    json recvjs = json::parse(buffer);
    if (recvjs["msgid"].get<int>() == REG_MSG_ACK && recvjs["errno"].get<int>() == 0)
    {
        LOG_INFO << "Register succeeded, userId=" << recvjs["id"].get<int>();
        return true;
    }

    LOG_WARN << "Register failed, reason=" << recvjs.value("errmsg", string(""));
    return false;
}

void ClientApp::mainMenu()
{
    _dispatcher.printHelp();
    cin.clear();
    if (cin.peek() == '\n')
    {
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    for (;;)
    {
        cout << "\n请输入您的选择：" << flush;
        string commandLine;
        if (!getline(cin, commandLine))
        {
            break;
        }
        if (!_dispatcher.execute(commandLine))
        {
            _session.clear();
            break;
        }
    }
}

void ClientApp::readLoop()
{
    for (;;)
    {
        char buffer[8192] = {0};
        int len = recv(_sockfd, buffer, sizeof(buffer), 0);
        if (len <= 0)
        {
            closeConnection();
            break;
        }
        handleIncoming(json::parse(buffer));
    }
}

void ClientApp::handleIncoming(const json &js)
{
    int msgtype = js["msgid"].get<int>();
    switch (msgtype)
    {
    case ONE_CHAT_MSG:
        printDirectMessage(js, "好友消息：");
        storeMessageIfPossible(js);
        break;
    case GROUP_CHAT_MSG:
        printGroupMessage(js, "群组消息：");
        storeMessageIfPossible(js);
        break;
    case ONE_CHAT_MSG_ACK:
        cout << js.value("errmsg", "") << endl;
        if (js.value("errno", 1) == 0 && js.contains("message"))
        {
            storeMessageIfPossible(js["message"]);
        }
        break;
    case GROUP_CHAT_MSG_ACK:
        cout << js.value("errmsg", "") << endl;
        if (js.value("errno", 1) == 0 && js.contains("message"))
        {
            storeMessageIfPossible(js["message"]);
        }
        break;
    case ADD_FRIEND_MSG_ACK:
    case CREATE_GROUP_MSG_ACK:
    case ADD_GROUP_MSG_ACK:
        cout << js.value("errmsg", "操作完成") << endl;
        break;
    default:
        LOG_WARN << "Received unhandled message: " << js.dump();
        break;
    }
}

void ClientApp::syncHistoryFromLogin(const json &recvjs)
{
    if (!recvjs.contains("history_messages") || !recvjs["history_messages"].is_array())
    {
        if (recvjs.contains("sync_cursor"))
        {
            _storage.updateSyncCursor(_session.currentUserId(), recvjs["sync_cursor"].get<long long>());
        }
        return;
    }

    vector<json> historyMessages = recvjs["history_messages"].get<vector<json>>();
    if (!historyMessages.empty())
    {
        _storage.saveMessages(_session.currentUserId(), historyMessages);
        LOG_INFO << "Sync history messages count=" << historyMessages.size()
                 << ", userId=" << _session.currentUserId();
    }
    if (recvjs.contains("sync_cursor"))
    {
        _storage.updateSyncCursor(_session.currentUserId(), recvjs["sync_cursor"].get<long long>());
    }
}

void ClientApp::storeMessageIfPossible(const json &message)
{
    if (!_session.isLoggedIn())
    {
        return;
    }
    _storage.saveMessage(_session.currentUserId(), message);
    long long currentCursor = _storage.getLastSyncCursor(_session.currentUserId());
    long long messageId = message.value("messageid", 0LL);
    if (messageId > currentCursor)
    {
        _storage.updateSyncCursor(_session.currentUserId(), messageId);
    }
}

void ClientApp::printDirectMessage(const json &js, const string &prefix)
{
    cout << prefix << "好友ID：" << js.value("fromid", js.value("id", -1))
         << " 时间：" << js.value("time", "")
         << " 消息内容：" << js.value("msg", "") << endl;
}

void ClientApp::printGroupMessage(const json &js, const string &prefix)
{
    cout << prefix << "群组ID：" << js.value("groupid", -1)
         << " 发送者ID：" << js.value("fromid", js.value("userid", -1))
         << " 时间：" << js.value("time", "")
         << " 消息内容：" << js.value("msg", "") << endl;
}

bool ClientApp::sendJson(const json &js)
{
    string payload = js.dump();
    return send(_sockfd, payload.c_str(), payload.size(), 0) > 0;
}
