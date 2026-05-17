#ifndef GAME_RECORD_MODEL_HPP
#define GAME_RECORD_MODEL_HPP

#include <string>
#include <vector>

/**
 * @file gameRecordModel.hpp
 * @brief 游戏对战记录数据访问层
 * 
 * 持久化对战记录到 MySQL，支持排行榜查询。
 */

struct GameRecord
{
    int id = 0;
    int player1Id = -1;
    int player2Id = -1;
    int player1Score = 0;
    int player2Score = 0;
    int winnerId = -1;
    int duration = 60; // 游戏时长(秒)
    double player1Accuracy = 0.0;
    double player2Accuracy = 0.0;
    double player1Wpm = 0.0;
    double player2Wpm = 0.0;
    int player1MaxCombo = 0;
    int player2MaxCombo = 0;
    std::string createdAt;
};

struct LeaderboardEntry
{
    int userId = -1;
    std::string username;
    int totalWins = 0;
    int totalGames = 0;
    double avgScore = 0.0;
    double avgAccuracy = 0.0;
    double avgWpm = 0.0;
};

class GameRecordModel
{
public:
    // 插入对战记录
    bool insert(const GameRecord &record);

    // 查询用户的对战历史
    std::vector<GameRecord> queryByUser(int userId, int limit = 20);

    // 查询排行榜
    std::vector<LeaderboardEntry> queryLeaderboard(int limit = 50);
};

#endif // GAME_RECORD_MODEL_HPP