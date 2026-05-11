# MySQL数据库表结构

## 1. user 表
- `id`：用户ID（主键，自增，INT）
- `name`：用户名（VARCHAR(64)）
- `password`：密码哈希（VARCHAR(128)，Argon2id格式）
- `state`：在线状态（ENUM：'online'/'offline'，默认'offline'）

## 2. friend 表
- `userid`：用户ID
- `friendid`：好友ID
- 主键：(userid, friendid) 联合主键

## 3. allgroup 表
- `id`：群组ID（主键，自增，INT）
- `groupname`：群名称（VARCHAR(128)）
- `groupdesc`：群描述（VARCHAR(200)，默认''）

## 4. groupuser 表
- `groupid`：群组ID
- `userid`：用户ID
- `grouprole`：群内角色（VARCHAR(20)，默认'normal'，可选值：'creator'/'normal'）
- 主键：(groupid, userid) 联合主键

## 5. offlinemessage 表
- `userid`：接收消息的用户ID
- `message`：消息内容（VARCHAR(6000)，JSON格式）

## 6. chatmessage 表
- `id`：消息ID（主键，自增，BIGINT）
- `session_type`：会话类型（ENUM：'single'/'group'）
- `from_userid`：发送者用户ID
- `to_userid`：接收者用户ID（单聊时有效，NULL表示群聊）
- `group_id`：群组ID（群聊时有效，NULL表示单聊）
- `content`：消息内容（TEXT类型）
- `msg_time`：消息时间戳（VARCHAR(32)）
- `created_at`：创建时间（TIMESTAMP，默认当前时间）
- 索引：idx_chatmessage_user, idx_chatmessage_group, idx_chatmessage_sender
