#ifndef GAME_ROOM_HPP
#define GAME_ROOM_HPP

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/EventLoop.h>
#include <nlohmann/json.hpp>
#include "server/game/appleSequencer.hpp"
#include "server/model/gameRecordModel.hpp"

/**
 * @file gameRoom.hpp
 * @brief 单个游戏房间，管理对局状态机
 * 
 * 状态机: Waiting → Countdown → Playing → Finished
 * 持有 AppleSequencer、双方玩家状态、活跃苹果列表。
 * 使用 Muduo Timer 驱动苹果生成和游戏逻辑。
 */

class GameRoom
{
public:
    enum class State
    {
        Waiting,
        Countdown,
        Playing,
        Finished
    };

    struct PlayerState
    {
        int userId = -1;
        std::string username;
        muduo::net::TcpConnectionPtr conn;
        bool ready = false;
        int score = 0;
        int lives = 3;
        int successCount = 0;
        int failureCount = 0;
        int maxCombo = 0;
        int currentCombo = 0;
        double accuracy = 0.0;
        double wpm = 0.0;
    };

    struct ActiveApple
    {
        int appleId = 0;        // 唯一苹果 ID（用于客户端精确匹配同步）
        char letter;
        float x;            // 归一化 X 位置 (0.0~1.0)
        int64_t spawnTime;  // 生成时间戳 (ms)
        int fallSpeed;      // 下落速度
        bool alive = true;
    };

    GameRoom(const std::string &roomId, const std::string &roomName, muduo::net::EventLoop *loop);
    ~GameRoom();

    // 房间管理
    const std::string &roomId() const { return m_roomId; }
    const std::string &roomName() const { return m_roomName; }
    State state() const { return m_state; }
    bool isFull() const;
    bool hasPlayer(int userId) const;

    // 玩家操作
    bool addPlayer(int userId, const std::string &username, const muduo::net::TcpConnectionPtr &conn);
    void removePlayer(int userId);
    void playerReady(int userId);

    // 游戏操作
    nlohmann::json handleKeyPress(int userId, char letter, int64_t timestamp, int requestedAppleId = -1);

    // 获取玩家信息
    const PlayerState *getPlayer(int userId) const;
    const PlayerState *getOpponent(int userId) const;
    const PlayerState *getPlayer1() const { return &m_player1; }
    const PlayerState *getPlayer2() const { return &m_player2; }

    // 获取活跃苹果
    const std::vector<ActiveApple> &getActiveApples() const { return m_activeApples; }

    // 获取游戏配置
    int gameDuration() const { return m_gameDuration; }

    // 持久化支持
    void setRecordModel(GameRecordModel* model) { m_recordModel = model; }
    GameRecordModel* recordModel() const { return m_recordModel; }

    // 游戏结束（可由 GameRoomManager 在玩家离开时调用）
    void endGame(const std::string &reason, int disconnectedUserId = -1);

    // 对局结束后自动清理（由 GameRoomManager 设置回调，在 queueInLoop 中安全执行）
    using GameEndedCallback = std::function<void()>;
    void setOnGameEnded(GameEndedCallback cb) { m_onGameEnded = std::move(cb); }

private:
    // 状态转换
    void startCountdown();
    void startGame();

    // 游戏逻辑
    void spawnApple();
    void checkAppleTimeout();
    void updateDifficulty(int elapsedSeconds);
    void broadcastToPlayers(const nlohmann::json &msg);
    void sendToPlayer(int userId, const nlohmann::json &msg);
    void broadcastOpponentState();

    // Muduo Timer 回调
    void onSpawnTimer();
    void onGameTimer();
    void onCountdownTimer();
    void onOpponentStateTimer();
    void onDifficultyCheckTimer();

    std::string m_roomId;
    std::string m_roomName;
    State m_state = State::Waiting;
    muduo::net::EventLoop *m_loop;

    PlayerState m_player1;
    PlayerState m_player2;
    int m_playerCount = 0;

    // 游戏状态
    AppleSequencer m_sequencer;
    std::vector<ActiveApple> m_activeApples;
    int m_nextAppleId = 1;     // 苹果 ID 自增计数器
    int64_t m_gameStartTime = 0;  // 游戏开始时间 (ms since epoch)
    int m_gameDuration = 60;      // 游戏时长 (秒)
    int m_countdownRemaining = 3; // 倒计时剩余秒数
    int m_currentDifficulty = 1;  // 当前难度等级

    // 游戏配置 (从 ConfigManager 读取)
    int m_initialFallSpeed = 2;
    int m_initialSpawnInterval = 1500;
    int m_maxActiveApples = 3;
    int m_initialLives = 3;

    // Muduo Timer IDs
    muduo::net::TimerId m_spawnTimerId;
    muduo::net::TimerId m_gameTimerId;
    muduo::net::TimerId m_countdownTimerId;
    muduo::net::TimerId m_opponentStateTimerId;
    muduo::net::TimerId m_difficultyTimerId;

    // 持久化
    GameRecordModel* m_recordModel = nullptr;

    // 对局结束回调（由 GameRoomManager 设置）
    GameEndedCallback m_onGameEnded;
};

#endif // GAME_ROOM_HPP
