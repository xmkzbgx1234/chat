#include "server/game/gameRoomManager.hpp"

#include "public.hpp"
#include "server/utils/responseBuilder.hpp"
#include "server/utils/errorCode.hpp"
#include "common/log.h"

#include <nlohmann/json.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

using namespace std;
using json = nlohmann::json;
using namespace muduo::net;
using namespace muduo;

// ============================================================================
// Singleton
// ============================================================================

GameRoomManager &GameRoomManager::instance()
{
    static GameRoomManager inst;
    return inst;
}

// ============================================================================
// Private helpers
// ============================================================================

string GameRoomManager::generateRoomId()
{
    auto now = chrono::system_clock::now();
    auto ms = chrono::duration_cast<chrono::milliseconds>(
        now.time_since_epoch()).count();
    return "room_" + to_string(++m_roomCounter) + "_" + to_string(ms);
}

// ============================================================================
// Room operations
// ============================================================================

string GameRoomManager::createRoom(int player1Id, const string &player1Name,
                                   const TcpConnectionPtr &conn,
                                   EventLoop *loop,
                                   const string &roomName)
{
    lock_guard<mutex> lock(m_mutex);

    string roomId = generateRoomId();

    string rn = roomName.empty() ? player1Name + "'s room" : roomName;

    auto room = make_unique<GameRoom>(roomId, rn, loop);
    room->addPlayer(player1Id, player1Name, conn);
    room->setRecordModel(m_recordModel);

    m_rooms[roomId] = move(room);
    m_playerRoomMap[player1Id] = roomId;

    LOG_INFO << "Room created: " << roomId << " by player " << player1Id
             << " name='" << player1Name << "'";

    // 房间已创建，handleCreateRoom 会发送响应
    return roomId;
}

json GameRoomManager::joinRoom(const string &roomId, int player2Id,
                               const string &player2Name,
                               const TcpConnectionPtr &conn)
{
    lock_guard<mutex> lock(m_mutex);

    auto it = m_rooms.find(roomId);
    if (it == m_rooms.end())
    {
        LOG_WARN << "joinRoom failed: room not found - " << roomId;
        return ResponseBuilder::error(GAME_JOIN_ROOM,
                                      ErrorCode::GAME_ROOM_NOT_FOUND);
    }

    GameRoom *room = it->second.get();
    if (room->isFull())
    {
        LOG_WARN << "joinRoom failed: room full - " << roomId;
        return ResponseBuilder::error(GAME_JOIN_ROOM,
                                      ErrorCode::GAME_ROOM_FULL);
    }

    room->addPlayer(player2Id, player2Name, conn);
    m_playerRoomMap[player2Id] = roomId;

    LOG_INFO << "Player " << player2Id << " joined room " << roomId;

    // Broadcast room state to all players in the room
    json stateMsg;
    stateMsg["msgid"]       = GAME_ROOM_STATE;
    stateMsg["roomId"]      = roomId;
    stateMsg["state"]       = "Waiting";
    stateMsg["playerCount"] = 2;

    const auto *p1 = room->getPlayer1();
    const auto *p2 = room->getPlayer2();
    if (p1)
    {
        stateMsg["player1Name"]  = p1->username;
        stateMsg["player1Id"]    = p1->userId;
        stateMsg["player1Ready"] = p1->ready;
    }
    if (p2)
    {
        stateMsg["player2Name"]  = p2->username;
        stateMsg["player2Id"]    = p2->userId;
        stateMsg["player2Ready"] = p2->ready;
    }

    string encodedState = encodeMessage(stateMsg.dump());
    if (p1 && p1->conn)
    {
        p1->conn->send(encodedState);
    }
    if (p2 && p2->conn)
    {
        p2->conn->send(encodedState);
    }

    // Build success response
    json success;
    success["msgid"]       = GAME_JOIN_ROOM;
    success["errno"]       = static_cast<int>(ErrorCode::SUCCESS);
    success["roomId"]      = roomId;
    success["roomName"]    = room->roomName();
    success["player2Name"] = player2Name;
    if (p1)
    {
        success["player1Name"] = p1->username;
    }

    return success;
}

json GameRoomManager::leaveRoom(int userId)
{
    lock_guard<mutex> lock(m_mutex);

    auto pit = m_playerRoomMap.find(userId);
    if (pit == m_playerRoomMap.end())
    {
        LOG_WARN << "leaveRoom failed: player " << userId << " not in any room";
        return ResponseBuilder::error(GAME_LEAVE_ROOM,
                                      ErrorCode::GAME_NOT_IN_ROOM);
    }

    string roomId = pit->second;
    auto rit = m_rooms.find(roomId);
    if (rit == m_rooms.end())
    {
        m_playerRoomMap.erase(pit);
        LOG_WARN << "leaveRoom failed: room " << roomId << " not found";
        return ResponseBuilder::error(GAME_LEAVE_ROOM,
                                      ErrorCode::GAME_ROOM_NOT_FOUND);
    }

    GameRoom *room = rit->second.get();
    GameRoom::State prevState = room->state();

    // 如果对局进行中，通过 endGame() 正常结束
    // (停止所有定时器、切换到 Finished 状态、广播完整 GAME_OVER、持久化记录)
    if (prevState == GameRoom::State::Playing)
    {
        room->endGame("opponent_disconnected", userId);
    }

    room->removePlayer(userId);
    m_playerRoomMap.erase(pit);

    LOG_INFO << "Player " << userId << " left room " << roomId;

    // Destroy room if both player slots are now empty
    if (room->getPlayer1()->userId == -1 && room->getPlayer2()->userId == -1)
    {
        m_rooms.erase(rit);
        LOG_INFO << "Room " << roomId << " destroyed (empty)";
    }

    json success;
    success["msgid"]  = GAME_LEAVE_ROOM;
    success["errno"]  = static_cast<int>(ErrorCode::SUCCESS);
    success["roomId"] = roomId;

    return success;
}

json GameRoomManager::getRoomList() const
{
    lock_guard<mutex> lock(m_mutex);

    json rooms = json::array();

    for (const auto &[id, room] : m_rooms)
    {
        if (room->state() == GameRoom::State::Waiting)
        {
            json info;
            info["roomId"]      = id;
            info["roomName"]    = room->roomName();
            info["player1Name"] = room->getPlayer1()
                                      ? room->getPlayer1()->username
                                      : "";
            info["playerCount"] = room->isFull() ? 2 : 1;
            info["state"]       = static_cast<int>(room->state());
            rooms.push_back(info);

            LOG_INFO << "getRoomList: room=" << id
                     << " player1Name='" << (room->getPlayer1() ? room->getPlayer1()->username : "null") << "'"
                     << " playerCount=" << (room->isFull() ? 2 : 1)
                     << " state=" << static_cast<int>(room->state());
        }
    }

    return rooms;
}

// ============================================================================
// Queries
// ============================================================================

GameRoom *GameRoomManager::getRoom(const string &roomId)
{
    lock_guard<mutex> lock(m_mutex);

    auto it = m_rooms.find(roomId);
    if (it != m_rooms.end())
    {
        return it->second.get();
    }
    return nullptr;
}

GameRoom *GameRoomManager::getRoomByPlayer(int userId)
{
    lock_guard<mutex> lock(m_mutex);

    auto pit = m_playerRoomMap.find(userId);
    if (pit == m_playerRoomMap.end())
    {
        return nullptr;
    }

    auto rit = m_rooms.find(pit->second);
    if (rit != m_rooms.end())
    {
        return rit->second.get();
    }
    return nullptr;
}

bool GameRoomManager::isInRoom(int userId) const
{
    lock_guard<mutex> lock(m_mutex);

    return m_playerRoomMap.find(userId) != m_playerRoomMap.end();
}

// ============================================================================
// Disconnect handling
// ============================================================================

void GameRoomManager::handleDisconnect(int userId)
{
    LOG_INFO << "Handling disconnect for player " << userId;
    leaveRoom(userId);
}
