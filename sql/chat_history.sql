CREATE TABLE IF NOT EXISTS chatmessage (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    session_type ENUM('single', 'group') NOT NULL,
    from_userid INT NOT NULL,
    to_userid INT DEFAULT NULL,
    group_id INT DEFAULT NULL,
    content TEXT NOT NULL,
    msg_time VARCHAR(32) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_chatmessage_user (to_userid, id),
    INDEX idx_chatmessage_group (group_id, id),
    INDEX idx_chatmessage_sender (from_userid, id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
