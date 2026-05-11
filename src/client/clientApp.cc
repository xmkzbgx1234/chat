#include "clientApp.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <limits>
#include <stdexcept>
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
    // 本地 SQLite 用来缓存聊天记录与同步游标，启动时必须先准备好。
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
    // 客户端与服务端保持一条长连接，后续所有请求和推送都复用这个 socket。
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

bool ClientApp::recvExact(void *buffer, size_t size)
{
    char *cursor = static_cast<char *>(buffer);
    size_t received = 0;
    while (received < size)
    {
        const ssize_t len = recv(_sockfd, cursor + received, size - received, 0);
        if (len <= 0)
        {
            return false;
        }
        received += static_cast<size_t>(len);
    }
    return true;
}

bool ClientApp::recvJson(json &js)
{
    uint32_t networkSize = 0;
    if (!recvExact(&networkSize, header_size))
    {
        return false;
    }

    const uint32_t bodySize = ntohl(networkSize);
    if (bodySize > max_body_size)
    {
        LOG_ERROR << "Received oversized packet, bodySize=" << bodySize;
        return false;
    }

    std::string payload(bodySize, '\0');
    if (bodySize > 0 && !recvExact(&payload[0], bodySize))
    {
        return false;
    }

    try
    {
        js = json::parse(payload);
        return true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Parse framed JSON failed: " << e.what();
        return false;
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
    // 带上本地最后一次同步到的 messageid，服务端只需要补发增量历史。
    request["sync_cursor"] = _storage.getLastSyncCursor(id);
    if (!sendJson(request))
    {
        LOG_ERROR << "Send login request failed, userId=" << id;
        return false;
    }

    json recvjs;
    if (!recvJson(recvjs))
    {
        LOG_ERROR << "Receive login response failed, userId=" << id;
        return false;
    }
    if (recvjs["msgid"].get<int>() != LOGIN_MSG_ACK || recvjs["errno"].get<int>() != 0)
    {
        LOG_WARN << "Login failed, userId=" << id << ", reason="
                 << recvjs.value("errmsg", string("未知错误"));
        return false;
    }

    LOG_INFO << "Login succeeded, userId=" << recvjs.value("id", id);
    _session.applyLogin(recvjs, id);
    // 登录成功后先把服务端返回的增量历史落库，再进入实时收发阶段。
    syncHistoryFromLogin(recvjs);
    _session.showCurrentUserInfo();
    
    // 处理服务器返回的离线消息，先展示，然后存入本地sqlite
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
            // 离线消息和实时消息统一走本地存储，保证会话记录完整。
            storeMessageIfPossible(offlineMessage);
        }
    }

    // 后台线程持续接收服务端推送，主线程继续处理用户输入命令。
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

    json recvjs;
    if (!recvJson(recvjs))
    {
        LOG_ERROR << "Receive register response failed";
        return false;
    }
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
        json incoming;
        if (!recvJson(incoming))
        {
            // 连接断开后退出读循环，避免后台线程持续空转。
            closeConnection();
            break;
        }
        handleIncoming(incoming);
    }
}

void ClientApp::handleIncoming(const json &js)
{
    int msgtype = js["msgid"].get<int>();
    switch (msgtype)
    {
    case ONE_CHAT_MSG:
        // 服务端主动推送的新私聊消息需要同时展示到终端并写入本地历史。
        printDirectMessage(js, "好友消息：");
        storeMessageIfPossible(js);
        break;
    case GROUP_CHAT_MSG:
        // 群消息和私聊消息共用同一套持久化策略，保证本地检索一致。
        printGroupMessage(js, "群组消息：");
        storeMessageIfPossible(js);
        break;
    case ONE_CHAT_MSG_ACK:
        cout << js.value("errmsg", "") << endl;
        if (js.value("errno", 1) == 0 && js.contains("message"))
        {
            // 发送成功回执里带回了最终入库消息，按服务端版本落库可避免本地状态不一致。
            storeMessageIfPossible(js["message"]);
        }
        break;
    case GROUP_CHAT_MSG_ACK:
        cout << js.value("errmsg", "") << endl;
        if (js.value("errno", 1) == 0 && js.contains("message"))
        {
            // 群消息发送成功后同样使用服务端确认过的消息对象更新本地记录。
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

// 同步服务器的历史聊天记录
void ClientApp::syncHistoryFromLogin(const json &recvjs)
{
    if (!recvjs.contains("history_messages") || !recvjs["history_messages"].is_array())
    {
        if (recvjs.contains("sync_cursor"))
        {
            // 即使本次没有历史消息，也要推进游标，避免下次重复拉取。
            _storage.updateSyncCursor(_session.currentUserId(), recvjs["sync_cursor"].get<long long>());
        }
        return;
    }

    vector<json> historyMessages = recvjs["history_messages"].get<vector<json>>();
    if (!historyMessages.empty())
    {
        // 批量落库比逐条写入更适合首次登录后的历史补偿场景。
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
        // 游标始终记录“已同步到哪一条消息”，供下次登录增量同步使用。
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
    const string packet = encodeMessage(js.dump());
    size_t totalSent = 0;
    while (totalSent < packet.size())
    {
        const ssize_t len = send(_sockfd, packet.data() + totalSent, packet.size() - totalSent, 0);
        if (len <= 0)
        {
            return false;
        }
        totalSent += static_cast<size_t>(len);
    }
    return true;
}
