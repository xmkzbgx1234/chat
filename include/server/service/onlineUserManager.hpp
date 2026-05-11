#ifndef ONLINE_USER_MANAGER_HPP
#define ONLINE_USER_MANAGER_HPP

// 在线用户管理器
// 用于管理在线用户的连接，包括添加、删除、获取用户连接等操作
#include <unordered_map>
#include <mutex>
#include <muduo/net/TcpConnection.h>
#include <memory>

#include "public.hpp"

using TcpConnectionPtr = std::shared_ptr<muduo::net::TcpConnection>;

class OnlineUserManager {
private:
    std::unordered_map<int, TcpConnectionPtr> _userConnMap;
    mutable std::mutex _mutex;  // mutable 允许 const 方法加锁

public:
    // 构造函数
    OnlineUserManager() = default;

    // 添加用户连接（返回旧连接，用于踢下线）
    TcpConnectionPtr addUser(int userid, const TcpConnectionPtr& conn) {
        std::lock_guard<std::mutex> lock(_mutex);
        TcpConnectionPtr oldConn = nullptr;
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end()) {
            oldConn = it->second;  // 保存旧连接
        }
        _userConnMap[userid] = conn;
        return oldConn;  // 返回旧连接（如果有）
    }

    // 删除用户连接
    void removeUser(int userid) {
        std::lock_guard<std::mutex> lock(_mutex);
        _userConnMap.erase(userid);
    }

    void clear() {
        std::lock_guard<std::mutex> lock(_mutex);
        _userConnMap.clear();
    }

    // 根据连接删除用户
    int removeByConn(const TcpConnectionPtr& conn) {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end();) {
            if (it->second == conn) {
                int userid = it->first;
                it = _userConnMap.erase(it);
                return userid;
            } else {
                ++it;
            }
        }
        return -1; // 未找到
    }

    // 获取用户连接（用于发消息）
    TcpConnectionPtr getUserConn(int userid) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end()) {
            return it->second; // 返回 shared_ptr 或 nullptr
        }
        return nullptr;
    }

    // 检查用户是否在线
    bool isOnline(int userid) const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _userConnMap.count(userid) > 0;
    }

    // 广播消息给所有在线用户（可选）
    void broadcast(const std::string& msg) const {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto& pair : _userConnMap) {
            if (pair.second) {
                pair.second->send(encodeMessage(msg));
            }
        }
    }
};

#endif // ONLINE_USER_MANAGER_HPP
