#include "server/model/gameRecordModel.hpp"
#include "server/db/commonConnectionPool.hpp"
#include "log.h"

#include <cstdio>

bool GameRecordModel::insert(const GameRecord &record)
{
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "INSERT INTO game_record (player1_id, player2_id, player1_score, player2_score, "
             "winner_id, duration, player1_accuracy, player2_accuracy, player1_wpm, player2_wpm, "
             "player1_max_combo, player2_max_combo) "
             "VALUES (%d, %d, %d, %d, %d, %d, %.2f, %.2f, %.2f, %.2f, %d, %d)",
             record.player1Id, record.player2Id, record.player1Score, record.player2Score,
             record.winnerId, record.duration,
             record.player1Accuracy, record.player2Accuracy,
             record.player1Wpm, record.player2Wpm,
             record.player1MaxCombo, record.player2MaxCombo);

    auto conn = ConnectionPool::getConnectionPool()->getConnection();
    if (conn && conn->update(sql))
    {
        LOG_INFO << "GameRecordModel::insert success: player1=" << record.player1Id
                 << " player2=" << record.player2Id
                 << " winner=" << record.winnerId;
        return true;
    }
    LOG_ERROR << "GameRecordModel::insert failed: "
              << (conn ? mysql_error(conn->getMySQL()) : "no connection");
    return false;
}

std::vector<GameRecord> GameRecordModel::queryByUser(int userId, int limit)
{
    char sql[512] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT id, player1_id, player2_id, player1_score, player2_score, "
             "winner_id, duration, player1_accuracy, player2_accuracy, "
             "player1_wpm, player2_wpm, player1_max_combo, player2_max_combo, created_at "
             "FROM game_record WHERE player1_id = %d OR player2_id = %d "
             "ORDER BY created_at DESC LIMIT %d",
             userId, userId, limit);

    std::vector<GameRecord> records;
    auto conn = ConnectionPool::getConnectionPool()->getConnection();
    if (conn)
    {
        MYSQL_RES *result = conn->query(sql);
        if (result)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result)))
            {
                GameRecord record;
                record.id = row[0] ? atoi(row[0]) : 0;
                record.player1Id = row[1] ? atoi(row[1]) : -1;
                record.player2Id = row[2] ? atoi(row[2]) : -1;
                record.player1Score = row[3] ? atoi(row[3]) : 0;
                record.player2Score = row[4] ? atoi(row[4]) : 0;
                record.winnerId = row[5] ? atoi(row[5]) : -1;
                record.duration = row[6] ? atoi(row[6]) : 60;
                record.player1Accuracy = row[7] ? atof(row[7]) : 0.0;
                record.player2Accuracy = row[8] ? atof(row[8]) : 0.0;
                record.player1Wpm = row[9] ? atof(row[9]) : 0.0;
                record.player2Wpm = row[10] ? atof(row[10]) : 0.0;
                record.player1MaxCombo = row[11] ? atoi(row[11]) : 0;
                record.player2MaxCombo = row[12] ? atoi(row[12]) : 0;
                record.createdAt = row[13] ? row[13] : "";
                records.push_back(record);
            }
            mysql_free_result(result);
        }
    }
    return records;
}

std::vector<LeaderboardEntry> GameRecordModel::queryLeaderboard(int limit)
{
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "SELECT u.id, u.name, "
             "SUM(CASE WHEN gr.winner_id = u.id THEN 1 ELSE 0 END) as total_wins, "
             "COUNT(*) as total_games, "
             "AVG(CASE WHEN gr.player1_id = u.id THEN gr.player1_score ELSE gr.player2_score END) as avg_score, "
             "AVG(CASE WHEN gr.player1_id = u.id THEN gr.player1_accuracy ELSE gr.player2_accuracy END) as avg_accuracy, "
             "AVG(CASE WHEN gr.player1_id = u.id THEN gr.player1_wpm ELSE gr.player2_wpm END) as avg_wpm "
             "FROM game_record gr "
             "JOIN user u ON (u.id = gr.player1_id OR u.id = gr.player2_id) "
             "GROUP BY u.id "
             "ORDER BY total_wins DESC, avg_score DESC "
             "LIMIT %d",
             limit);

    std::vector<LeaderboardEntry> entries;
    auto conn = ConnectionPool::getConnectionPool()->getConnection();
    if (conn)
    {
        MYSQL_RES *result = conn->query(sql);
        if (result)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result)))
            {
                LeaderboardEntry entry;
                entry.userId = row[0] ? atoi(row[0]) : -1;
                entry.username = row[1] ? row[1] : "";
                entry.totalWins = row[2] ? atoi(row[2]) : 0;
                entry.totalGames = row[3] ? atoi(row[3]) : 0;
                entry.avgScore = row[4] ? atof(row[4]) : 0.0;
                entry.avgAccuracy = row[5] ? atof(row[5]) : 0.0;
                entry.avgWpm = row[6] ? atof(row[6]) : 0.0;
                entries.push_back(entry);
            }
            mysql_free_result(result);
        }
        else
        {
            LOG_ERROR << "GameRecordModel::queryLeaderboard failed: " << mysql_error(conn->getMySQL());
        }
    }
    return entries;
}
