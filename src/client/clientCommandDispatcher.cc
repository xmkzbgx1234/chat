#include "clientCommandDispatcher.hpp"

#include <ctime>
#include <iostream>

#include "log.h"
#include "public.hpp"

using namespace std;
using json = nlohmann::json;

ClientCommandDispatcher::ClientCommandDispatcher(ClientSession &session, LocalChatStorage &storage, SendJson sendJson)
    : _session(session), _storage(storage), _sendJson(sendJson),
      _commandDescMap({
          {"help", "显示帮助信息,输入help显示帮助信息"},
          {"chat", "好友聊天,输入chat:friendid:msg 进行好友聊天"},
          {"groupchat", "群组聊天,输入groupchat:groupid:msg 进行群组聊天"},
          {"addfriend", "添加好友,输入addfriend:friendid 添加好友"},
          {"creategroup", "创建群组,输入creategroup:groupname:groupdesc 创建群组"},
          {"addgroup", "添加群组,输入addgroup:groupid 添加群组"},
          {"history", "查看本地聊天记录,输入history:single:friendid 或 history:group:groupid"},
          {"loginout", "退出登录,输入loginout 退出登录"},
      }),
      _handlers({
          {"help", [this](const string &params) { return handleHelp(params); }},
          {"chat", [this](const string &params) { return handleChat(params); }},
          {"groupchat", [this](const string &params) { return handleGroupChat(params); }},
          {"addfriend", [this](const string &params) { return handleAddFriend(params); }},
          {"creategroup", [this](const string &params) { return handleCreateGroup(params); }},
          {"addgroup", [this](const string &params) { return handleAddGroup(params); }},
          {"history", [this](const string &params) { return handleHistory(params); }},
          {"loginout", [this](const string &params) { return handleLogout(params); }},
      })
{}

void ClientCommandDispatcher::printHelp() const
{
    cout << "帮助信息：" << endl;
    for (const auto &entry : _commandDescMap)
    {
        cout << entry.first << " " << entry.second << endl;
    }
}

bool ClientCommandDispatcher::execute(const string &commandLine)
{
    string cmd = trim(commandLine);
    if (cmd.empty())
    {
        cout << "命令不能为空，请重新输入！" << endl;
        return true;
    }

    size_t pos = cmd.find(':');
    string command = pos == string::npos ? cmd : cmd.substr(0, pos);
    string params = pos == string::npos ? "" : cmd.substr(pos + 1);

    auto it = _handlers.find(command);
    if (it == _handlers.end())
    {
        cout << "输入错误！命令 \"" << command << "\" 不存在，请重新输入！" << endl;
        return true;
    }
    return it->second(params);
}

bool ClientCommandDispatcher::handleHelp(const string &)
{
    printHelp();
    return true;
}

bool ClientCommandDispatcher::handleChat(const string &params)
{
    size_t pos = params.find(':');
    if (pos == string::npos)
    {
        cout << "格式错误，应为 chat:friendid:msg" << endl;
        return true;
    }

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = _session.currentUserId();
    js["name"] = _session.currentUserName();
    js["toid"] = stoi(params.substr(0, pos));
    js["msg"] = params.substr(pos + 1);
    js["time"] = currentTime();
    if (!_sendJson(js))
    {
        LOG_ERROR << "Send single chat request failed, toid=" << js["toid"].get<int>();
    }
    return true;
}

bool ClientCommandDispatcher::handleGroupChat(const string &params)
{
    size_t pos = params.find(':');
    if (pos == string::npos)
    {
        cout << "格式错误，应为 groupchat:groupid:msg" << endl;
        return true;
    }

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["userid"] = _session.currentUserId();
    js["name"] = _session.currentUserName();
    js["groupid"] = stoi(params.substr(0, pos));
    js["msg"] = params.substr(pos + 1);
    js["time"] = currentTime();
    if (!_sendJson(js))
    {
        LOG_ERROR << "Send group chat request failed, groupid=" << js["groupid"].get<int>();
    }
    return true;
}

bool ClientCommandDispatcher::handleAddFriend(const string &params)
{
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["userid"] = _session.currentUserId();
    js["friendid"] = stoi(params);
    if (!_sendJson(js))
    {
        LOG_ERROR << "Send add-friend request failed, friendid=" << js["friendid"].get<int>();
    }
    return true;
}

bool ClientCommandDispatcher::handleCreateGroup(const string &params)
{
    size_t pos = params.find(':');
    if (pos == string::npos)
    {
        cout << "格式错误，应为 creategroup:groupname:groupdesc" << endl;
        return true;
    }

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["userid"] = _session.currentUserId();
    js["groupname"] = params.substr(0, pos);
    js["groupdesc"] = params.substr(pos + 1);
    if (!_sendJson(js))
    {
        LOG_ERROR << "Send create-group request failed, groupname=" << js["groupname"].get<string>();
    }
    return true;
}

bool ClientCommandDispatcher::handleAddGroup(const string &params)
{
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["userid"] = _session.currentUserId();
    js["groupid"] = stoi(params);
    if (!_sendJson(js))
    {
        LOG_ERROR << "Send add-group request failed, groupid=" << js["groupid"].get<int>();
    }
    return true;
}

bool ClientCommandDispatcher::handleHistory(const string &params)
{
    size_t pos = params.find(':');
    if (pos == string::npos)
    {
        cout << "格式错误，应为 history:single:friendid 或 history:group:groupid" << endl;
        return true;
    }

    string sessionType = params.substr(0, pos);
    int peerId = stoi(params.substr(pos + 1));
    vector<json> messages = _storage.querySessionMessages(_session.currentUserId(), sessionType, peerId);
    if (messages.empty())
    {
        cout << "本地暂无聊天记录" << endl;
        return true;
    }

    for (const auto &message : messages)
    {
        if (sessionType == "group")
        {
            printGroupMessage(message, "本地记录：");
        }
        else
        {
            printDirectMessage(message, "本地记录：");
        }
    }
    return true;
}

bool ClientCommandDispatcher::handleLogout(const string &)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["userid"] = _session.currentUserId();
    if (!_sendJson(js))
    {
        LOG_ERROR << "Send logout request failed, userId=" << _session.currentUserId();
        return true;
    }
    LOG_INFO << "Logout succeeded, userId=" << _session.currentUserId();
    return false;
}

string ClientCommandDispatcher::currentTime()
{
    time_t now = time(nullptr);
    char buf[64] = {0};
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return string(buf);
}

void ClientCommandDispatcher::printDirectMessage(const json &js, const string &prefix)
{
    cout << prefix << "好友ID：" << js.value("fromid", js.value("id", -1))
         << " 时间：" << js.value("time", "")
         << " 消息内容：" << js.value("msg", "") << endl;
}

void ClientCommandDispatcher::printGroupMessage(const json &js, const string &prefix)
{
    cout << prefix << "群组ID：" << js.value("groupid", -1)
         << " 发送者ID：" << js.value("fromid", js.value("userid", -1))
         << " 时间：" << js.value("time", "")
         << " 消息内容：" << js.value("msg", "") << endl;
}

string ClientCommandDispatcher::trim(const string &text)
{
    size_t first = text.find_first_not_of(" \t");
    if (first == string::npos)
    {
        return "";
    }
    size_t last = text.find_last_not_of(" \t");
    return text.substr(first, last - first + 1);
}
