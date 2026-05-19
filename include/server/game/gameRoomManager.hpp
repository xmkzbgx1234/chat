#ifndef GAME_ROOM_MANAGER_HPP
#define GAME_ROOM_MANAGER_HPP

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>
#include <muduo/net/TcpConnection.h>
#include <nlohmann/json.hpp>
#include "server/game/gameRoom.hpp"
#include "server/model/gameRecordModel.hpp"

/**
 * @file gameRoomManager.hpp
 * @brief 游戏房间管理器，管理所有游戏房间
 * 
 * 线程安全的房间创建/加入/离开/查询/销毁。
 */

class GameRoomManager
{
public:
    static GameRoomManager &instance();

    // 房间操作
    std::string createRoom(int player1Id, const std::string &player1Name,
                           const muduo::net::TcpConnectionPtr &conn,
                           muduo::net::EventLoop *loop,
                           const std::string &roomName = "");

    nlohmann::json joinRoom(const std::string &roomId, int player2Id,
                            const std::string &player2Name,
                            const muduo::net::TcpConnectionPtr &conn);

    nlohmann::json leaveRoom(int userId);

    nlohmann::json getRoomList() const;

    // 查询
    GameRoom *getRoom(const std::string &roomId);
    GameRoom *getRoomByPlayer(int userId);
    bool isInRoom(int userId) const;

    // 断线处理
    void handleDisconnect(int userId);

    // 持久化支持
    void setRecordModel(GameRecordModel* model) { m_recordModel = model; }

private:
    GameRoomManager() = default;
    std::string generateRoomId();

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::unique_ptr<GameRoom>> m_rooms;
    std::unordered_map<int, std::string> m_playerRoomMap; // userId -> roomId
    int m_roomCounter = 0;
    GameRecordModel* m_recordModel = nullptr;
};

#endif // GAME_ROOM_MANAGER_HPP