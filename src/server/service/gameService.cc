#include "server/service/gameService.hpp"
#include "server/game/gameRoomManager.hpp"
#include "server/model/gameRecordModel.hpp"
#include "server/utils/responseBuilder.hpp"
#include "server/utils/errorCode.hpp"
#include "public.hpp"
#include "log.h"

using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

GameService::GameService(GameRoomManager &roomManager, GameRecordModel &recordModel)
    : m_roomManager(roomManager)
    , m_recordModel(recordModel)
{
}

void GameService::handleMessage(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    if (!js.contains("msgid") || !js["msgid"].is_number_integer())
    {
        conn->send(encodeMessage(ResponseBuilder::error(-1, ErrorCode::INVALID_PARAMS, "无效的消息ID").dump()));
        return;
    }

    int msgid = js["msgid"].get<int>();

    switch (msgid)
    {
    case GAME_CREATE_ROOM:
        handleCreateRoom(conn, js, time);
        break;
    case GAME_JOIN_ROOM:
        handleJoinRoom(conn, js, time);
        break;
    case GAME_LEAVE_ROOM:
        handleLeaveRoom(conn, js, time);
        break;
    case GAME_ROOM_LIST:
        handleRoomList(conn, js, time);
        break;
    case GAME_READY:
        handleReady(conn, js, time);
        break;
    case GAME_KEY_PRESS:
        handleKeyPress(conn, js, time);
        break;
    case GAME_LEADERBOARD:
        handleLeaderboard(conn, js, time);
        break;
    default:
        conn->send(encodeMessage(ResponseBuilder::error(msgid, ErrorCode::INVALID_PARAMS, "未知游戏消息类型").dump()));
        break;
    }
}

void GameService::handleCreateRoom(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = js.value("userid", -1);
    std::string username = js.value("username", "");
    std::string roomName = js.value("roomName", "Game Room");

    if (userId == -1 || username.empty())
    {
        conn->send(encodeMessage(ResponseBuilder::error(GAME_CREATE_ROOM, ErrorCode::INVALID_PARAMS, "缺少用户信息").dump()));
        return;
    }

    // 检查用户是否已在房间中
    if (m_roomManager.isInRoom(userId))
    {
        conn->send(encodeMessage(ResponseBuilder::error(GAME_CREATE_ROOM, ErrorCode::GAME_ALREADY_IN_ROOM, "已在房间中").dump()));
        return;
    }

    std::string roomId = m_roomManager.createRoom(userId, username, conn, conn->getLoop(), roomName);

    json response = ResponseBuilder::success(GAME_CREATE_ROOM, "房间创建成功");
    response["roomId"] = roomId;
    response["roomName"] = roomName;
    response["player1Name"] = username;
    conn->send(encodeMessage(response.dump()));

    LOG_INFO << "GameService: user " << userId << " created room " << roomId;
}

void GameService::handleJoinRoom(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = js.value("userid", -1);
    std::string username = js.value("username", "");
    std::string roomId = js.value("roomId", "");

    if (userId == -1 || username.empty() || roomId.empty())
    {
        conn->send(encodeMessage(ResponseBuilder::error(GAME_JOIN_ROOM, ErrorCode::INVALID_PARAMS, "缺少必要参数").dump()));
        return;
    }

    if (m_roomManager.isInRoom(userId))
    {
        conn->send(encodeMessage(ResponseBuilder::error(GAME_JOIN_ROOM, ErrorCode::GAME_ALREADY_IN_ROOM, "已在房间中").dump()));
        return;
    }

    json result = m_roomManager.joinRoom(roomId, userId, username, conn);
    conn->send(encodeMessage(result.dump()));

    LOG_INFO << "GameService: user " << userId << " joined room " << roomId;
}

void GameService::handleLeaveRoom(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = js.value("userid", -1);

    if (userId == -1)
    {
        conn->send(encodeMessage(ResponseBuilder::error(GAME_LEAVE_ROOM, ErrorCode::INVALID_PARAMS, "缺少用户ID").dump()));
        return;
    }

    json result = m_roomManager.leaveRoom(userId);
    conn->send(encodeMessage(result.dump()));

    LOG_INFO << "GameService: user " << userId << " left room";
}

void GameService::handleRoomList(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    json response = ResponseBuilder::success(GAME_ROOM_LIST, "房间列表");
    response["rooms"] = m_roomManager.getRoomList();
    conn->send(encodeMessage(response.dump()));
}

void GameService::handleReady(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = js.value("userid", -1);
    std::string roomId = js.value("roomId", "");

    if (userId == -1 || roomId.empty())
    {
        conn->send(encodeMessage(ResponseBuilder::error(GAME_READY, ErrorCode::INVALID_PARAMS, "缺少必要参数").dump()));
        return;
    }

    std::shared_ptr<GameRoom> room = m_roomManager.getRoom(roomId);
    if (!room)
    {
        conn->send(encodeMessage(ResponseBuilder::error(GAME_READY, ErrorCode::GAME_ROOM_NOT_FOUND, "房间不存在").dump()));
        return;
    }

    room->playerReady(userId);

    json response = ResponseBuilder::success(GAME_READY, "准备就绪");
    conn->send(encodeMessage(response.dump()));

    LOG_INFO << "GameService: user " << userId << " ready in room " << roomId;
}

void GameService::handleKeyPress(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userId = js.value("userid", -1);
    std::string roomId = js.value("roomId", "");
    std::string letter = js.value("letter", "");
    int64_t timestamp = js.value("timestamp", 0);
    int requestedAppleId = js.value("appleId", -1);

    if (userId == -1 || roomId.empty() || letter.empty())
    {
        conn->send(encodeMessage(ResponseBuilder::error(GAME_KEY_PRESS, ErrorCode::INVALID_PARAMS, "缺少必要参数").dump()));
        return;
    }

    std::shared_ptr<GameRoom> room = m_roomManager.getRoom(roomId);
    if (!room)
    {
        conn->send(encodeMessage(ResponseBuilder::error(GAME_KEY_PRESS, ErrorCode::GAME_ROOM_NOT_FOUND, "房间不存在").dump()));
        return;
    }

    // handleKeyPress 内部已经发送了 HIT_RESULT 和 SCORE_UPDATE
    room->handleKeyPress(userId, letter[0], timestamp, requestedAppleId);
}

int GameService::getUserIdFromConn(const TcpConnectionPtr &conn)
{
    // 通过 OnlineUserManager 查找，但 GameService 不直接持有
    // 实际使用中从消息 JSON 中获取 userid
    return -1;
}

void GameService::handleDisconnect(int userId)
{
    m_roomManager.handleDisconnect(userId);
}

void GameService::handleLeaderboard(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int limit = js.value("limit", 50);

    auto entries = m_recordModel.queryLeaderboard(limit);
    json response = ResponseBuilder::success(GAME_LEADERBOARD_ACK, "排行榜数据");
    json leaderboard = json::array();
    for (const auto &entry : entries)
    {
        json item;
        item["userId"] = entry.userId;
        item["username"] = entry.username;
        item["totalWins"] = entry.totalWins;
        item["totalGames"] = entry.totalGames;
        item["avgScore"] = entry.avgScore;
        item["avgAccuracy"] = entry.avgAccuracy;
        item["avgWpm"] = entry.avgWpm;
        leaderboard.push_back(item);
    }
    response["leaderboard"] = leaderboard;
    conn->send(encodeMessage(response.dump()));
}
