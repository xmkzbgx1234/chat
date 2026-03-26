#include "clientSession.hpp"

#include <iostream>

using namespace std;
using json = nlohmann::json;

void ClientSession::applyLogin(const json &recvjs, int fallbackId)
{
    _currentUser.setId(recvjs.value("id", fallbackId));
    _currentUser.setName(recvjs.value("name", ""));
    _currentUser.setState(recvjs.value("state", "online"));

    _friendList.clear();
    if (recvjs.contains("friends") && recvjs["friends"].is_array())
    {
        for (const json &friendJson : recvjs["friends"])
        {
            User friendUser;
            friendUser.setId(friendJson.value("id", -1));
            friendUser.setName(friendJson.value("name", ""));
            friendUser.setState(friendJson.value("state", "offline"));
            _friendList.push_back(friendUser);
        }
    }

    _groupList.clear();
    if (recvjs.contains("groups") && recvjs["groups"].is_array())
    {
        for (const json &groupJson : recvjs["groups"])
        {
            Group group;
            group.setName(groupJson.value("name", ""));
            group.setId(groupJson.value("id", -1));
            group.setDesc(groupJson.value("desc", ""));
            if (groupJson.contains("users") && groupJson["users"].is_array())
            {
                for (const json &userJson : groupJson["users"])
                {
                    groupUser member;
                    member.setId(userJson.value("id", -1));
                    member.setName(userJson.value("name", ""));
                    member.setState(userJson.value("state", "offline"));
                    member.setRole(userJson.value("role", "normal"));
                    group.getUsers().push_back(member);
                }
            }
            _groupList.push_back(group);
        }
    }
}

void ClientSession::clear()
{
    _currentUser = User();
    _friendList.clear();
    _groupList.clear();
}

bool ClientSession::isLoggedIn() const
{
    return _currentUser.getId() > 0;
}

int ClientSession::currentUserId() const
{
    return _currentUser.getId();
}

std::string ClientSession::currentUserName() const
{
    return _currentUser.getName();
}

void ClientSession::showCurrentUserInfo() const
{
    cout << "当前登录用户信息：" << endl;
    cout << "用户ID：" << _currentUser.getId() << endl;
    cout << "用户名：" << _currentUser.getName() << endl;
    cout << "用户状态：" << _currentUser.getState() << endl;

    for (const User &friendUser : _friendList)
    {
        cout << "好友ID：" << friendUser.getId() << endl;
        cout << "好友用户名：" << friendUser.getName() << endl;
        cout << "好友状态：" << friendUser.getState() << endl;
    }

    for (Group group : _groupList)
    {
        cout << "群组ID：" << group.getId() << endl;
        cout << "群组名称：" << group.getName() << endl;
        cout << "群组描述：" << group.getDesc() << endl;
        for (const groupUser &member : group.getUsers())
        {
            cout << "群组用户ID：" << member.getId() << endl;
            cout << "群组用户名：" << member.getName() << endl;
            cout << "群组用户状态：" << member.getState() << endl;
            cout << "群组用户角色：" << member.getRole() << endl;
        }
    }
}
