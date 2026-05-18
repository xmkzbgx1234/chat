#include "server/game/gameRoom.hpp"
#include "server/utils/configManager.hpp"
#include "server/utils/responseBuilder.hpp"
#include "server/utils/errorCode.hpp"
#include "server/model/gameRecordModel.hpp"
#include "public.hpp"
#include "log.h"

#include <chrono>
#include <random>
#include <cmath>

using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

GameRoom::GameRoom(const std::string &roomId, const std::string &roomName, EventLoop *loop)
    : m_roomId(roomId)
    , m_roomName(roomName)
    , m_loop(loop)
    , m_playerCount(0)
    , m_gameDuration(60)
    , m_countdownRemaining(3)
    , m_currentDifficulty(1)
{
    // 从配置读取游戏参数
    ConfigManager *config = ConfigManager::getInstance();
    m_initialFallSpeed = config->getInt("game", "initialFallSpeed", 2);
    m_initialSpawnInterval = config->getInt("game", "initialSpawnInterval", 1500);
    m_maxActiveApples = config->getInt("game", "maxActiveApples", 3);
    m_initialLives = config->getInt("game", "initialLives", 3);
    m_gameDuration = config->getInt("game", "gameDuration", 60);

    m_player1.userId = -1;
    m_player2.userId = -1;

    LOG_INFO << "GameRoom created: " << roomId;
}

GameRoom::~GameRoom()
{
    // 取消所有定时器
    if (m_loop)
    {
        m_loop->cancel(m_spawnTimerId);
        m_loop->cancel(m_gameTimerId);
        m_loop->cancel(m_countdownTimerId);
        m_loop->cancel(m_opponentStateTimerId);
        m_loop->cancel(m_difficultyTimerId);
    }
    LOG_INFO << "GameRoom destroyed: " << m_roomId;
}

bool GameRoom::isFull() const
{
    return m_playerCount >= 2;
}

bool GameRoom::hasPlayer(int userId) const
{
    return m_player1.userId == userId || m_player2.userId == userId;
}

bool GameRoom::addPlayer(int userId, const std::string &username, const TcpConnectionPtr &conn)
{
    if (isFull() || hasPlayer(userId))
    {
        return false;
    }

    if (m_player1.userId == -1)
    {
        m_player1.userId = userId;
        m_player1.username = username;
        m_player1.conn = conn;
        m_player1.ready = false;
        m_player1.score = 0;
        m_player1.lives = m_initialLives;
    }
    else
    {
        m_player2.userId = userId;
        m_player2.username = username;
        m_player2.conn = conn;
        m_player2.ready = false;
        m_player2.score = 0;
        m_player2.lives = m_initialLives;
    }
    m_playerCount++;
    LOG_INFO << "GameRoom " << m_roomId << ": player " << userId << " (" << username << ") joined";
    return true;
}

void GameRoom::removePlayer(int userId)
{
    if (m_player1.userId == userId)
    {
        m_player1 = PlayerState{};  // 完全重置，避免残留旧数据
        m_playerCount--;
    }
    else if (m_player2.userId == userId)
    {
        m_player2 = PlayerState{};
        m_playerCount--;
    }
}

void GameRoom::playerReady(int userId)
{
    if (m_player1.userId == userId)
    {
        m_player1.ready = true;
    }
    else if (m_player2.userId == userId)
    {
        m_player2.ready = true;
    }

    LOG_INFO << "GameRoom " << m_roomId << ": player " << userId << " ready";

    // 双方都准备后开始倒计时
    if (m_player1.ready && m_player2.ready && m_state == State::Waiting)
    {
        startCountdown();
    }
}

nlohmann::json GameRoom::handleKeyPress(int userId, char letter, int64_t timestamp)
{
    if (m_state != State::Playing)
    {
        return ResponseBuilder::error(GAME_KEY_PRESS, ErrorCode::GAME_ALREADY_STARTED);
    }

    PlayerState *player = nullptr;
    PlayerState *opponent = nullptr;
    if (m_player1.userId == userId)
    {
        player = &m_player1;
        opponent = &m_player2;
    }
    else if (m_player2.userId == userId)
    {
        player = &m_player2;
        opponent = &m_player1;
    }
    else
    {
        return ResponseBuilder::error(GAME_KEY_PRESS, ErrorCode::GAME_NOT_IN_ROOM);
    }

    // 查找匹配的活跃苹果
    bool hit = false;
    for (auto &apple : m_activeApples)
    {
        if (apple.alive && apple.letter == letter)
        {
            apple.alive = false;
            hit = true;
            player->score += 10;
            player->successCount++;
            player->currentCombo++;
            if (player->currentCombo > player->maxCombo)
            {
                player->maxCombo = player->currentCombo;
            }
            break;
        }
    }

    if (!hit)
    {
        player->failureCount++;
        player->currentCombo = 0;
    }

    // 计算准确率
    int total = player->successCount + player->failureCount;
    player->accuracy = total > 0 ? (double)player->successCount / total * 100.0 : 0.0;

    // 发送命中结果给玩家
    json hitResult;
    hitResult["msgid"] = GAME_HIT_RESULT;
    hitResult["hit"] = hit;
    hitResult["letter"] = std::string(1, letter);
    hitResult["score"] = player->score;
    hitResult["lives"] = player->lives;
    sendToPlayer(userId, hitResult);

    // 如果命中，通知对手
    if (hit && opponent && opponent->userId != -1)
    {
        json opponentHit;
        opponentHit["msgid"] = GAME_OPPONENT_HIT;
        opponentHit["letter"] = std::string(1, letter);
        opponentHit["opponentScore"] = player->score;
        sendToPlayer(opponent->userId, opponentHit);
    }

    // 广播分数更新
    json scoreUpdate;
    scoreUpdate["msgid"] = GAME_SCORE_UPDATE;
    scoreUpdate["roomId"] = m_roomId;
    scoreUpdate["player1Score"] = m_player1.score;
    scoreUpdate["player2Score"] = m_player2.score;
    scoreUpdate["player1Lives"] = m_player1.lives;
    scoreUpdate["player2Lives"] = m_player2.lives;
    broadcastToPlayers(scoreUpdate);

    return hitResult;
}

const GameRoom::PlayerState *GameRoom::getPlayer(int userId) const
{
    if (m_player1.userId == userId)
    {
        return &m_player1;
    }
    if (m_player2.userId == userId)
    {
        return &m_player2;
    }
    return nullptr;
}

const GameRoom::PlayerState *GameRoom::getOpponent(int userId) const
{
    if (m_player1.userId == userId)
    {
        return &m_player2;
    }
    if (m_player2.userId == userId)
    {
        return &m_player1;
    }
    return nullptr;
}

void GameRoom::startCountdown()
{
    m_state = State::Countdown;
    m_countdownRemaining = 3;
    LOG_INFO << "GameRoom " << m_roomId << ": countdown started";

    // 广播倒计时开始
    json stateMsg;
    stateMsg["msgid"] = GAME_ROOM_STATE;
    stateMsg["roomId"] = m_roomId;
    stateMsg["state"] = "Countdown";
    stateMsg["player1Name"] = m_player1.username;
    stateMsg["player2Name"] = m_player2.username;
    stateMsg["countdown"] = m_countdownRemaining;
    broadcastToPlayers(stateMsg);

    // 每秒倒计时
    m_countdownTimerId = m_loop->runEvery(1.0, [this]() {
        onCountdownTimer();
    });
}

void GameRoom::startGame()
{
    m_state = State::Playing;
    m_gameStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();

    // 初始化苹果序列生成器
    std::random_device rd;
    uint32_t seed = rd();
    m_sequencer.initialize(seed, 1);
    m_sequencer.setFallSpeed(m_initialFallSpeed);
    m_sequencer.setSpawnInterval(m_initialSpawnInterval);
    m_sequencer.setMaxActiveApples(m_maxActiveApples);

    // 重置玩家状态
    m_player1.score = 0;
    m_player1.lives = m_initialLives;
    m_player1.successCount = 0;
    m_player1.failureCount = 0;
    m_player1.maxCombo = 0;
    m_player1.currentCombo = 0;

    m_player2.score = 0;
    m_player2.lives = m_initialLives;
    m_player2.successCount = 0;
    m_player2.failureCount = 0;
    m_player2.maxCombo = 0;
    m_player2.currentCombo = 0;

    m_activeApples.clear();
    m_currentDifficulty = 1;

    LOG_INFO << "GameRoom " << m_roomId << ": game started, seed=" << seed;

    // 广播游戏开始
    json startMsg;
    startMsg["msgid"] = GAME_START;
    startMsg["roomId"] = m_roomId;
    startMsg["seed"] = seed;
    startMsg["level"] = 1;
    startMsg["maxApples"] = m_maxActiveApples;
    startMsg["fallSpeed"] = m_initialFallSpeed;
    startMsg["spawnInterval"] = m_initialSpawnInterval;
    startMsg["duration"] = m_gameDuration;
    startMsg["opponentName"] = m_player2.username;
    sendToPlayer(m_player1.userId, startMsg);

    startMsg["opponentName"] = m_player1.username;
    sendToPlayer(m_player2.userId, startMsg);

    // 启动定时器
    ConfigManager *config = ConfigManager::getInstance();
    int spawnInterval = config->getInt("game", "initialSpawnInterval", 1500);
    int syncInterval = config->getInt("game", "opponentSyncInterval", 200);

    m_spawnTimerId = m_loop->runEvery(spawnInterval / 1000.0, [this]() {
        onSpawnTimer();
    });
    m_gameTimerId = m_loop->runAfter(m_gameDuration, [this]() {
        onGameTimer();
    });
    m_opponentStateTimerId = m_loop->runEvery(syncInterval / 1000.0, [this]() {
        onOpponentStateTimer();
    });
    m_difficultyTimerId = m_loop->runEvery(1.0, [this]() {
        onDifficultyCheckTimer();
    });
}

void GameRoom::endGame(const std::string &reason)
{
    m_state = State::Finished;

    // 取消所有定时器
    m_loop->cancel(m_spawnTimerId);
    m_loop->cancel(m_gameTimerId);
    m_loop->cancel(m_countdownTimerId);
    m_loop->cancel(m_opponentStateTimerId);
    m_loop->cancel(m_difficultyTimerId);

    // 计算WPM (假设平均单词长度5)
    int elapsed = m_gameDuration; // 简化：使用配置的游戏时长
    m_player1.wpm = elapsed > 0 ? (double)m_player1.successCount / 5.0 / (double)elapsed * 60.0 : 0.0;
    m_player2.wpm = elapsed > 0 ? (double)m_player2.successCount / 5.0 / (double)elapsed * 60.0 : 0.0;

    // 确定胜者
    int winnerId = -1;
    if (reason == "lives_zero")
    {
        winnerId = (m_player1.lives <= 0) ? m_player2.userId : m_player1.userId;
    }
    else if (reason == "opponent_disconnected")
    {
        winnerId = (m_player1.userId == -1) ? m_player2.userId : m_player1.userId;
    }
    else
    {
        // 时间到，比分数
        winnerId = (m_player1.score > m_player2.score) ? m_player1.userId : m_player2.userId;
    }

    LOG_INFO << "GameRoom " << m_roomId << ": game ended, reason=" << reason
             << ", winner=" << winnerId;

    // 广播游戏结束
    json overMsg;
    overMsg["msgid"] = GAME_OVER;
    overMsg["roomId"] = m_roomId;
    overMsg["reason"] = reason;
    overMsg["winner"] = winnerId;
    overMsg["player1Score"] = m_player1.score;
    overMsg["player2Score"] = m_player2.score;
    overMsg["player1Accuracy"] = m_player1.accuracy;
    overMsg["player2Accuracy"] = m_player2.accuracy;
    overMsg["player1Wpm"] = m_player1.wpm;
    overMsg["player2Wpm"] = m_player2.wpm;
    overMsg["player1MaxCombo"] = m_player1.maxCombo;
    overMsg["player2MaxCombo"] = m_player2.maxCombo;
    overMsg["duration"] = m_gameDuration;
    broadcastToPlayers(overMsg);

    // 持久化对战记录 (由 GameService 调用 GameRecordModel)
}

void GameRoom::spawnApple()
{
    if (m_state != State::Playing)
    {
        return;
    }

    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    int64_t elapsed = now - m_gameStartTime;

    AppleSpawnInfo info = m_sequencer.nextApple(elapsed);
    ActiveApple apple;
    apple.letter = info.letter;
    apple.x = info.x;
    apple.spawnTime = info.spawnTime;
    apple.fallSpeed = info.fallSpeed;
    apple.alive = true;
    m_activeApples.push_back(apple);

    // 广播苹果生成
    json spawnMsg;
    spawnMsg["msgid"] = GAME_APPLE_SPAWN;
    spawnMsg["roomId"] = m_roomId;
    spawnMsg["letter"] = std::string(1, apple.letter);
    spawnMsg["x"] = apple.x;
    spawnMsg["spawnTime"] = apple.spawnTime;
    spawnMsg["fallSpeed"] = apple.fallSpeed;
    broadcastToPlayers(spawnMsg);
}

void GameRoom::checkAppleTimeout()
{
    if (m_state != State::Playing)
    {
        return;
    }

    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

    // 假设画布高度为 600 像素，苹果掉落到底部的时间 = 600 / fallSpeed * 16ms (约60fps)
    // 简化：使用固定超时时间
    int64_t fallDuration = 10000; // 10秒超时

    for (auto &apple : m_activeApples)
    {
        if (apple.alive && (now - apple.spawnTime) > fallDuration)
        {
            apple.alive = false;
            // 苹果掉底，双方都扣生命（各自独立）
            // 实际上应该只扣对应玩家的生命，但简化处理
        }
    }

    // 清理不活跃的苹果
    m_activeApples.erase(
        std::remove_if(m_activeApples.begin(), m_activeApples.end(),
                       [](const ActiveApple &a)
                       { return !a.alive; }),
        m_activeApples.end());
}

void GameRoom::updateDifficulty(int elapsedSeconds)
{
    ConfigManager *config = ConfigManager::getInstance();
    std::string milestones = config->getString("game", "difficultyMilestones", "20,40,50");
    std::string speedPercents = config->getString("game", "speedIncreasePercent", "20,40,60");
    std::string spawnPercents = config->getString("game", "spawnDecreasePercent", "20,35,50");

    // 简化：在特定时间点增加难度
    int newLevel = 1;
    if (elapsedSeconds >= 50)
        newLevel = 4;
    else if (elapsedSeconds >= 40)
        newLevel = 3;
    else if (elapsedSeconds >= 20)
        newLevel = 2;

    if (newLevel != m_currentDifficulty)
    {
        m_currentDifficulty = newLevel;
        m_sequencer.setLevel(newLevel);

        // 更新生成间隔
        int newInterval = config->getInt("game", "initialSpawnInterval", 1500);
        if (newLevel >= 2)
            newInterval = newInterval * 0.8;
        if (newLevel >= 3)
            newInterval = newInterval * 0.65;
        if (newLevel >= 4)
            newInterval = newInterval * 0.5;
        int minInterval = config->getInt("game", "minSpawnInterval", 500);
        newInterval = std::max(minInterval, newInterval);
        m_sequencer.setSpawnInterval(newInterval);

        // 重新设置生成定时器
        m_loop->cancel(m_spawnTimerId);
        m_spawnTimerId = m_loop->runEvery(newInterval / 1000.0, [this]() {
            onSpawnTimer();
        });

        LOG_INFO << "GameRoom " << m_roomId << ": difficulty increased to level " << newLevel
                 << ", spawnInterval=" << newInterval;
    }

    // 更新最大同屏苹果数
    if (elapsedSeconds >= 45)
    {
        m_sequencer.setMaxActiveApples(7);
    }
    else if (elapsedSeconds >= 30)
    {
        m_sequencer.setMaxActiveApples(5);
    }
}

void GameRoom::broadcastToPlayers(const json &msg)
{
    std::string data = encodeMessage(msg.dump());
    if (m_player1.conn)
    {
        m_player1.conn->send(data);
    }
    if (m_player2.conn)
    {
        m_player2.conn->send(data);
    }
}

void GameRoom::sendToPlayer(int userId, const json &msg)
{
    std::string data = encodeMessage(msg.dump());
    if (m_player1.userId == userId && m_player1.conn)
    {
        m_player1.conn->send(data);
    }
    else if (m_player2.userId == userId && m_player2.conn)
    {
        m_player2.conn->send(data);
    }
}

void GameRoom::broadcastOpponentState()
{
    if (m_state != State::Playing)
    {
        return;
    }

    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

    // 向玩家1发送玩家2的状态
    if (m_player1.conn && m_player2.userId != -1)
    {
        json stateMsg;
        stateMsg["msgid"] = GAME_OPPONENT_STATE;
        stateMsg["roomId"] = m_roomId;
        stateMsg["score"] = m_player2.score;
        stateMsg["lives"] = m_player2.lives;

        json apples = json::array();
        for (const auto &apple : m_activeApples)
        {
            if (apple.alive)
            {
                // 计算苹果当前Y位置 (归一化 0.0~1.0)
                int64_t elapsed = now - apple.spawnTime;
                double y = std::min(1.0, (double)elapsed / 10000.0); // 10秒到底
                json appleJson;
                appleJson["letter"] = std::string(1, apple.letter);
                appleJson["x"] = apple.x;
                appleJson["y"] = y;
                appleJson["state"] = "falling";
                apples.push_back(appleJson);
            }
        }
        stateMsg["apples"] = apples;
        sendToPlayer(m_player1.userId, stateMsg);
    }

    // 向玩家2发送玩家1的状态
    if (m_player2.conn && m_player1.userId != -1)
    {
        json stateMsg;
        stateMsg["msgid"] = GAME_OPPONENT_STATE;
        stateMsg["roomId"] = m_roomId;
        stateMsg["score"] = m_player1.score;
        stateMsg["lives"] = m_player1.lives;

        json apples = json::array();
        for (const auto &apple : m_activeApples)
        {
            if (apple.alive)
            {
                int64_t elapsed = now - apple.spawnTime;
                double y = std::min(1.0, (double)elapsed / 10000.0);
                json appleJson;
                appleJson["letter"] = std::string(1, apple.letter);
                appleJson["x"] = apple.x;
                appleJson["y"] = y;
                appleJson["state"] = "falling";
                apples.push_back(appleJson);
            }
        }
        stateMsg["apples"] = apples;
        sendToPlayer(m_player2.userId, stateMsg);
    }
}

void GameRoom::onSpawnTimer()
{
    spawnApple();
    checkAppleTimeout();
}

void GameRoom::onGameTimer()
{
    endGame("time_up");
}

void GameRoom::onCountdownTimer()
{
    m_countdownRemaining--;
    if (m_countdownRemaining <= 0)
    {
        m_loop->cancel(m_countdownTimerId);
        // 发送 GO! 消息
        json stateMsg;
        stateMsg["msgid"] = GAME_ROOM_STATE;
        stateMsg["roomId"] = m_roomId;
        stateMsg["state"] = "Playing";
        stateMsg["countdown"] = 0;
        broadcastToPlayers(stateMsg);
        startGame();
        return;
    }

    json stateMsg;
    stateMsg["msgid"] = GAME_ROOM_STATE;
    stateMsg["roomId"] = m_roomId;
    stateMsg["state"] = "Countdown";
    stateMsg["countdown"] = m_countdownRemaining;
    broadcastToPlayers(stateMsg);
}

void GameRoom::onOpponentStateTimer()
{
    broadcastOpponentState();
}

void GameRoom::onDifficultyCheckTimer()
{
    if (m_state != State::Playing)
    {
        return;
    }

    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    int elapsed = static_cast<int>(now - m_gameStartTime / 1000);
    updateDifficulty(elapsed);
}