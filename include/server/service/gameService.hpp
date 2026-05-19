#ifndef GAME_SERVICE_HPP
#define GAME_SERVICE_HPP

#include <muduo/net/TcpConnection.h>
#include <nlohmann/json.hpp>
#include "server/service/baseService.hpp"
#include "server/game/gameRoomManager.hpp"
#include "server/model/gameRecordModel.hpp"

/**
 * @file gameService.hpp
 * @brief 游戏业务服务，处理所有 GAME_* 消息
 * 
 * 继承 BaseService，通过 msgid 路由到具体处理方法。
 */

class GameService : public BaseService
{
public:
    GameService(GameRoomManager &roomManager, GameRecordModel &recordModel);
    ~GameService() = default;

    void handleMessage(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time) override;

private:
    // 房间管理
    void handleCreateRoom(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time);
    void handleJoinRoom(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time);
    void handleLeaveRoom(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time);
    void handleRoomList(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time);

    // 游戏流程
    void handleReady(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time);
    void handleKeyPress(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time);
    void handleLeaderboard(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time);

    // 断线处理
    void handleDisconnect(int userId);

    // 辅助方法
    int getUserIdFromConn(const muduo::net::TcpConnectionPtr &conn);

    GameRoomManager &m_roomManager;
    GameRecordModel &m_recordModel;
};

#endif // GAME_SERVICE_HPP