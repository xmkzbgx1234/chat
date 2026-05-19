-- 游戏对战记录表
CREATE TABLE IF NOT EXISTS game_record (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    player1_id INT NOT NULL,
    player2_id INT NOT NULL,
    player1_score INT DEFAULT 0,
    player2_score INT DEFAULT 0,
    winner_id INT DEFAULT -1,
    duration INT DEFAULT 60 COMMENT '游戏时长(秒)',
    player1_accuracy DOUBLE DEFAULT 0.0 COMMENT '玩家1准确率(%)',
    player2_accuracy DOUBLE DEFAULT 0.0 COMMENT '玩家2准确率(%)',
    player1_wpm DOUBLE DEFAULT 0.0 COMMENT '玩家1WPM',
    player2_wpm DOUBLE DEFAULT 0.0 COMMENT '玩家2WPM',
    player1_max_combo INT DEFAULT 0 COMMENT '玩家1最高连击',
    player2_max_combo INT DEFAULT 0 COMMENT '玩家2最高连击',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_game_record_player1 (player1_id),
    INDEX idx_game_record_player2 (player2_id),
    INDEX idx_game_record_winner (winner_id),
    INDEX idx_game_record_created (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 游戏排行榜视图
CREATE OR REPLACE VIEW game_leaderboard AS
SELECT 
    u.id AS user_id,
    u.name AS username,
    SUM(CASE WHEN gr.winner_id = u.id THEN 1 ELSE 0 END) AS total_wins,
    COUNT(*) AS total_games,
    ROUND(AVG(CASE WHEN gr.player1_id = u.id THEN gr.player1_score ELSE gr.player2_score END), 2) AS avg_score,
    ROUND(AVG(CASE WHEN gr.player1_id = u.id THEN gr.player1_accuracy ELSE gr.player2_accuracy END), 2) AS avg_accuracy,
    ROUND(AVG(CASE WHEN gr.player1_id = u.id THEN gr.player1_wpm ELSE gr.player2_wpm END), 2) AS avg_wpm
FROM game_record gr
JOIN user u ON (u.id = gr.player1_id OR u.id = gr.player2_id)
GROUP BY u.id, u.name
ORDER BY total_wins DESC, avg_score DESC;