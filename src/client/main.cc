#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>
#include <thread>

#include "public.hpp"
#include "group.hpp"
#include "user.hpp"

using namespace std;
using nlohmann::json;

// 记录当前登录用户的信息
User g_currentUser;
// 记录当前登录用户的好友列表
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表
vector<Group> g_currentUserGroupList;

// 显示当前登录用户的基本信息
void showCurrentUserInfo();

// 接收线程
void readTaskHandler(int sockfd);

// 获取系统时间 (聊天信息需要添加时间信息)
string getSystemTime();

// 主聊天页面程序
void mainMenu(int sockfd);

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cerr << "命令格式错误！正确格式：./chat_client 127.0.0.1 6000" << endl;
        exit(-1);
    }
    string ip = argv[1];
    int port = atoi(argv[2]);
    // 创建客户端socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        cerr << "创建客户端socket失败！" << endl;
        exit(-1);
    }
    // 连接服务器
    sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serveraddr.sin_addr);
    if (connect(sockfd, (sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        cerr << "连接服务器失败！" << endl;
        exit(-1);
    }
    for (;;)
    {
        cout << "1. 登录" << endl;
        cout << "2. 注册" << endl;
        cout << "3. 退出" << endl;
        cout << "请输入您的选择：" << endl;
        int choice = 0;
        if (!(cin >> choice))
        {
            // 输入不是整数
            cin.clear();                                         // 清除错误状态
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // 丢弃整行
            cout << "输入无效，请输入数字！" << endl;
            continue; // 跳过本次循环，重新显示菜单
        }
        int len = 0;
        int msgtype = 0;
        switch (choice)
        {
        case 1:
        {
            // 登录
            // 从服务器接收登录成功或失败的消息
            int id = 0;
            char password[50] = {0};
            cout << "请输入账号：" << endl;
            cin >> id;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "请输入密码：" << endl;
            cin.getline(password, sizeof(password));
            // 序列化json字符串
            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = password;
            string jsstr = js.dump();
            len = send(sockfd, jsstr.c_str(), jsstr.size(), 0);
            if (len <= 0)
            {
                cerr << "send error 登录失败！" << endl;
                break;
            }
            // 从服务器接收登录成功或失败的消息
            char buffer[1024] = {0};
            len = recv(sockfd, buffer, sizeof(buffer), 0);
            if (len <= 0)
            {
                cerr << "recv error登录失败！" << endl;
                break;
            }
            json recvjs;
            // 反序列化json字符串
            recvjs = json::parse(buffer);
            msgtype = recvjs["msgid"].get<int>();
            if (msgtype == LOGIN_MSG_ACK && recvjs["errno"].get<int>() == 0)
            {
                // 登录成功
                cout << "登录成功！" << endl;
                // 记录当前登录用户的信息
                g_currentUser.setId(id);
                g_currentUser.setName(recvjs["name"]);

                // 记录当前登录用户的好友列表
                auto &friends = recvjs["friends"]; // nlohmann::json 支持直接遍历 array

                // 如果字段不存在，friends 会是 null，需检查
                if (!friends.is_array())
                {
                    cout << "No valid friends list." << endl;
                }
                else
                {
                    g_currentUserFriendList.clear(); // 清空旧数据
                    for (const auto &f : friends)
                    {
                        // f 是一个 JSON 对象：{"id":2, "name":"lisi", "state":"offline"}
                        User frienduser;
                        frienduser.setId(f.value("id", -1));
                        frienduser.setName(f.value("name", ""));
                        frienduser.setState(f.value("state", "offline"));
                        g_currentUserFriendList.push_back(frienduser);
                    }
                }
                // 记录当前登录用户的群组列表
                auto &groups = recvjs["groups"]; // nlohmann::json 支持直接遍历 array

                // 如果字段不存在，groups 会是 null，需检查
                if (!groups.is_array())
                {
                    cout << "No valid groups list." << endl;
                }
                else
                {
                    g_currentUserGroupList.clear(); // 清空旧数据
                    for (const auto &g : groups)
                    {
                        // g 是一个 JSON 对象：{"id":1, "name":"group1", "desc":"group1 desc", "users":[{"id":1, "name":"zhangsan", "state":"online", "role":1}]}
                        Group group;
                        group.setName(g.value("name", ""));
                        group.setId(g.value("id", -1));
                        group.setDesc(g.value("desc", ""));
                        for (auto &user_ : g["users"])
                        {
                            groupUser groupuser;
                            groupuser.setId(user_["id"]);
                            groupuser.setName(user_["name"]);
                            groupuser.setState(user_["state"]);
                            groupuser.setRole(user_["role"]);
                            group.getUsers().push_back(groupuser);
                        }
                        g_currentUserGroupList.push_back(group);
                    }
                    showCurrentUserInfo();
                    if (recvjs.contains("offlinemsg"))
                    {
                        vector<string> offlinemsgjs = recvjs["offlinemsg"];
                        // 遍历离线消息，将消息存储到_offLineMsgModel中
                        for (auto &offlinemsg_ : offlinemsgjs)
                        {
                            json offlinemsg = json::parse(offlinemsg_);
                            cout << "离线消息：" << "time:" << offlinemsg["time"]
                                 << "id:" << offlinemsg["id"] << " msg:" << offlinemsg["msg"] << endl;
                        }
                    }
                }
                // 启动接收线程
                thread readTask(readTaskHandler, sockfd);
                readTask.detach();
                mainMenu(sockfd);
            }
            else
            {
                // errno=1 用户名或密码错误 errno=2 该账号已登陆，不能重复登录
                cerr << "登录失败！" << recvjs["errmsg"] << endl;
            }
            break;
        }
        case 2:
        {
            // 注册
            // 从服务器接收注册成功或失败的消息
            char name[50] = {0};
            char password[50] = {0};
            cout << "请输入用户名：" << endl;
            cin.getline(name, sizeof(name));
            cout << "请输入密码：" << endl;
            cin.getline(password, sizeof(password));
            // 序列化json字符串
            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = password;
            string jsstr = js.dump();
            len = send(sockfd, jsstr.c_str(), jsstr.size(), 0);
            if (len <= 0)
            {
                cerr << "注册失败！" << jsstr << endl;
                break;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(sockfd, buffer, sizeof(buffer), 0);
                if (len <= 0)
                {
                    cerr << "注册失败！" << jsstr << endl;
                    break;
                }
                // 反序列化json字符串
                json recvjs;
                recvjs = json::parse(buffer);
                msgtype = recvjs["msgid"].get<int>();
                if (msgtype == REG_MSG_ACK && recvjs["errno"].get<int>() == 0)
                {
                    // 注册成功
                    cout << "注册成功，用户ID为：" << recvjs["id"].get<int>() << endl;
                }
                else
                {
                    // 注册失败
                    cerr << "用户名已存在，注册失败！" << recvjs["errmsg"].get<string>() << endl;
                }
            }
            break;
        }
        case 3:
        {
            close(sockfd);
            exit(0);
        }
        default:
        {
            cout << "输入错误，请重新输入！" << endl;
            break;
        }
        }
    }
}

void readTaskHandler(int sockfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(sockfd, buffer, sizeof(buffer), 0);
        if (len <= 0)
        {
            close(sockfd);
            break;
        }
        // 反序列化json字符串
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (msgtype == ONE_CHAT_MSG)
        {
            // 好友消息
            cout << "好友消息：" << "好友ID：" << js["id"] << " 好友用户名：" << js["name"] 
            << " 时间：" << js["time"] << " 消息内容：" << js["msg"] << endl;
        }
        else if (msgtype == GROUP_CHAT_MSG)
        {
            // 群组消息
            cout << "群组消息：" << "群组ID：" << js["groupid"] << " 时间：" << js["time"]
             << " 发送者ID：" << js["userid"] << " 消息内容：" << js["msg"] << endl;
        }
    }
}

void showCurrentUserInfo()
{
    cout << "当前登录用户信息：" << endl;
    cout << "用户ID：" << g_currentUser.getId() << endl;
    cout << "用户名：" << g_currentUser.getName() << endl;
    cout << "用户状态：" << g_currentUser.getState() << endl;

    for (auto &frienduser : g_currentUserFriendList)
    {
        cout << "好友ID：" << frienduser.getId() << endl;
        cout << "好友用户名：" << frienduser.getName() << endl;
        cout << "好友状态：" << frienduser.getState() << endl;
    }

    for (auto &group : g_currentUserGroupList)
    {
        cout << "群组ID：" << group.getId() << endl;
        cout << "群组名称：" << group.getName() << endl;
        cout << "群组描述：" << group.getDesc() << endl;
        for (auto &groupuser : group.getUsers())
        {
            cout << "群组用户ID：" << groupuser.getId() << endl;
            cout << "群组用户名：" << groupuser.getName() << endl;
            cout << "群组用户状态：" << groupuser.getState() << endl;
            cout << "群组用户角色：" << groupuser.getRole() << endl;
        }
    }
}

string getSystemTime()
{
    time_t now = time(0);
    char buf[64] = {0};
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return string(buf);
}

void help(int sockfd, string msg);
void chat(int sockfd, string msg);
void groupchat(int sockfd, string msg);
void addfriend(int sockfd, string msg);
void creategroup(int sockfd, string msg);
void addgroup(int sockfd, string msg);
void loginout(int sockfd, string msg);

unordered_map<string, string> commandDescMap = {
    {"help", "显示帮助信息,输入help显示帮助信息"},
    {"chat", "好友聊天,输入chat:friendid:msg 进行好友聊天"},
    {"groupchat", "群组聊天,输入groupchat:groupid:msg 进行群组聊天"},
    {"addfriend", "添加好友,输入addfriend:friendid 添加好友"},
    {"creategroup", "创建群组,输入craetegroup:groupname:groupdesc 创建群组"},
    {"addgroup", "添加群组,输入addgroup:groupid 添加群组"},
    {"loginout", "退出登录,输入loginout 退出登录"},
};

unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"groupchat", groupchat},
    {"addfriend", addfriend}, 
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"loginout", loginout},
};

void mainMenu(int sockfd)
{
    help(sockfd, "");
    char buffer[1024] = {0};
    // 处理 cin 输入缓冲区残留的换行符（避免首次 getline 读取空）
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    for(;;) {
        cout << "\n请输入您的选择：" << flush; // flush 确保提示语立即输出
        cin.getline(buffer, sizeof(buffer));
        string cmd = buffer;
        // 去除命令前后的空白字符（处理用户输入的空格，如 " login:zhangsan:123 "）
        cmd.erase(0, cmd.find_first_not_of(" \t"));
        cmd.erase(cmd.find_last_not_of(" \t") + 1);
        if (cmd.empty()) { // 处理空输入
            cout << "命令不能为空，请重新输入！" << endl;
            continue;
        }

        string cmdType;    // 命令类型（如 login）
        string cmdParam;   // 命令参数（如 zhangsan:123）
        int pos = cmd.find(":");

        // 解析命令类型和参数（统一逻辑，避免分支混乱）
        if (pos == -1) {
            // 无冒号：命令类型=完整输入，参数为空
            cmdType = cmd;
            cmdParam = "";
            commandHandlerMap[cmdType](sockfd, cmdParam);
            if(cmdType == "loginout"){
                break;
            }
        } else {
            // 有冒号：命令类型=冒号前，参数=冒号后（简化 substr，默认到末尾）
            cmdType = cmd.substr(0, pos);
            cmdParam = cmd.substr(pos + 1);
        }

        // 检查命令是否存在（先 find 再调用，避免 Map 插入空函数）
        auto it = commandHandlerMap.find(cmdType);
        if (it != commandHandlerMap.end()) {
            // 执行命令处理函数（传入 sockfd + 参数）
            it->second(sockfd, cmdParam);
            // 特殊处理退出命令（比如 "exit" 命令）
        } else {
            cout << "输入错误！命令 \"" << cmdType << "\" 不存在，请重新输入！" << endl;
            continue;
        }
    }
}

void help(int sockfd, string msg)
{
    cout << "帮助信息：" << endl;
    for (auto &it : commandDescMap)
    {
        cout << it.first << " " << it.second << endl;
    }
}

void addfriend(int sockfd, string msg)
{
    int friendid = stoi(msg);
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["userid"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string req = js.dump();
    int len = send(sockfd, req.c_str(), req.size(), 0);
    if (len <= 0)
    {
        cout << "添加好友失败！" << endl;
    }
    else
    {
        cout << "添加好友成功！" << endl;
    }
}

void chat(int sockfd, string msg)
{
    int friendid = stoi(msg.substr(0, msg.find(":")));
    string friendmsg = msg.substr(msg.find(":") + 1, msg.size() - msg.find(":"));
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = friendmsg;
    js["time"] = getSystemTime();
    string req = js.dump();
    int len = send(sockfd, req.c_str(), req.size(), 0);
    if (len <= 0)
    {
        cout << "聊天失败！" << endl;
    }
    else
    {
        cout << "发送聊天成功！" << endl;
    }
}

void groupchat(int sockfd, string msg)
{
    int groupid = stoi(msg.substr(0, msg.find(":")));
    string groupmsg = msg.substr(msg.find(":") + 1, msg.size() - msg.find(":"));
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["userid"] = g_currentUser.getId();
    js["groupid"] = groupid;
    js["msg"] = groupmsg;
    js["time"] = getSystemTime();
    string req = js.dump();
    int len = send(sockfd, req.c_str(), req.size(), 0);
    if (len <= 0)
    {
        cout << "聊天失败！" << endl;
    }
    else
    {
        cout << "发送聊天成功！" << endl;
    }
}

void creategroup(int sockfd, string msg)
{
    string groupname = msg.substr(0, msg.find(":"));
    string groupdesc = msg.substr(msg.find(":") + 1, msg.size() - msg.find(":"));
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["userid"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string req = js.dump();
    int len = send(sockfd, req.c_str(), req.size(), 0);
    if (len <= 0)
    {
        cout << "创建群组失败！" << endl;
    }
    else
    {
        cout << "创建群组成功！" << endl;
    }
}

void addgroup(int sockfd, string msg)
{
    int groupid = stoi(msg);
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["userid"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string req = js.dump();
    int len = send(sockfd, req.c_str(), req.size(), 0);
    if (len <= 0)
    {
        cout << "添加群组失败！" << endl;
    }
    else
    {
        cout << "添加群组成功！" << endl;
    }
}

void loginout(int sockfd, string msg)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["userid"] = g_currentUser.getId();
    string req = js.dump();
    int len = send(sockfd, req.c_str(), req.size(), 0);
    if (len <= 0)
    {
        cout << "退出登录失败！" << endl;
    }
    else
    {
        cout << "退出登录成功！" << endl;
    }
    // 清空当前用户信息
    g_currentUser = User();
}

