#ifndef BASE_SERVICE_HPP
#define BASE_SERVICE_HPP

#include <nlohmann/json.hpp>
#include <muduo/net/TcpConnection.h>

class BaseService {
public:
    virtual ~BaseService() = default;
    virtual void handleMessage(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time) = 0;
};

#endif
