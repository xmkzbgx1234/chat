#ifndef GROUP_SERVICE_HPP
#define GROUP_SERVICE_HPP

#include "groupModel.hpp"
#include "baseService.hpp"
#include <muduo/net/TcpConnection.h>
#include <nlohmann/json.hpp>

class GroupService : public BaseService
{
private:
    GroupModel& _groupModel;

public:
    GroupService(GroupModel& groupModel);
    ~GroupService() = default;

    void handleMessage(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time) override;
    // 处理加入群组消息
    void addGroup(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
    // 处理创建群组消息
    void createGroup(const muduo::net::TcpConnectionPtr& conn, nlohmann::json &js, muduo::Timestamp time);
};
#endif
