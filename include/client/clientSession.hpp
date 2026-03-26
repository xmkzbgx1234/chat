#ifndef CLIENT_SESSION_HPP
#define CLIENT_SESSION_HPP

#include <vector>

#include <nlohmann/json.hpp>

#include "group.hpp"
#include "user.hpp"

class ClientSession {
public:
    void applyLogin(const nlohmann::json &recvjs, int fallbackId);
    void clear();
    bool isLoggedIn() const;
    int currentUserId() const;
    std::string currentUserName() const;
    void showCurrentUserInfo() const;

private:
    User _currentUser;
    std::vector<User> _friendList;
    std::vector<Group> _groupList;
};

#endif
