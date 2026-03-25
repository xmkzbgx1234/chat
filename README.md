# Cluster Chat Server

一个基于 `Muduo + MySQL + Redis + JSON` 的 C++ 集群聊天服务器项目，支持用户注册登录、好友聊天、群组聊天、离线消息存储，以及多服务器节点间的消息转发。

这个项目的目标不是做一个完整 IM 产品，而是把一个典型高并发后端服务里常见的几类能力串起来：

- 基于 `Muduo` 的 Reactor 网络模型
- 基于消息号的业务分发与服务解耦
- 基于 `Redis Pub/Sub` 的跨节点通信
- 基于 `MySQL` 的用户/好友/群组/离线消息持久化
- 基于连接池的数据库访问优化

## 项目亮点

- 支持单聊、群聊、离线消息、好友管理、群组管理等基础聊天能力
- 通过 `Redis` 发布订阅机制实现集群节点之间的消息转发
- 通过 `IService` 将网络层和业务层解耦，按 `msgid` 分发请求
- 使用自定义 `MySQL` 连接池，降低频繁建连开销
- 客户端与服务端通过 `JSON` 协议通信，便于调试和扩展
- 服务端基于 `Muduo` 多线程网络库，具备较好的并发处理基础

## 技术栈

- C++11
- Muduo
- MySQL / mysqlclient
- Redis / hiredis
- nlohmann/json
- CMake
- pthread

## 快速开始

推荐在 Linux 环境下完成依赖安装、编译和运行：

```bash
git clone https://github.com/xmkzbgx1234/chat.git
cd chat
cmake -S . -B build
cmake --build build -j
```

编译完成后，可执行文件默认输出到 `bin/` 目录：

- `bin/chat_server`
- `bin/chat_client`

## 功能列表

当前代码已实现的核心能力：

- 用户注册
- 用户登录 / 退出登录
- 重复登录校验
- 好友添加
- 单聊消息
- 群组创建
- 加入群组
- 群聊消息
- 离线消息存储与登录后拉取
- 集群节点间消息转发
- 异常断开后的在线状态重置

## 整体架构

### 1. 模块划分

```text
client
  -> 负责命令行交互、JSON 请求封装、接收服务端消息

chatServer
  -> 负责连接管理、收发数据、回调业务层

IService
  -> 单例服务路由中心，按 msgid 分发到不同业务模块

AuthService
  -> 注册、登录、退出

FriendService
  -> 添加好友、好友关系查询

GroupService
  -> 创建群组、加入群组

ChatService
  -> 单聊、群聊、离线消息处理、跨节点消息投递

Model
  -> UserModel / FriendModel / GroupModel / OffLineMsgModel

Redis
  -> 集群消息转发

ConnectionPool
  -> MySQL 连接池
```

### 2. 消息流转

1. 客户端将命令封装成 JSON 并发送给服务端
2. `ChatServer` 收到数据后解析 `msgid`
3. `IService` 根据 `msgid` 路由到对应业务模块
4. 业务模块执行数据库操作、在线用户管理或 Redis 转发
5. 目标用户在线则直接投递；跨节点在线则通过 Redis 发布；不在线则写入离线消息表

### 3. 集群通信思路

当用户连接到不同服务器节点时：

- 当前节点先检查目标用户是否连接在本机
- 如果不在本机，但数据库状态为在线，则通过 `Redis Pub/Sub` 向目标用户频道发布消息
- 目标用户所在节点订阅到消息后，转发给对应 TCP 连接
- 如果目标用户不在线，则写入离线消息表，等待下次登录拉取

## 目录结构

```text
chat/
|- include/
|  |- common/               # 通用数据对象
|  `- server/               # 服务端头文件
|- src/
|  |- client/               # 命令行客户端
|  |- common/               # 通用实体实现
|  `- server/
|     |- db/                # MySQL 封装与连接池
|     |- model/             # 数据访问层
|     |- redis/             # Redis 发布订阅封装
|     `- service/           # 业务服务层
|- test/
|  `- testmuduo/            # Muduo 学习/测试代码
|- build/                   # 构建产物
|- bin/                     # 可执行文件（当前仓库中已存在产物）
`- README.md
```

## 核心协议

项目通过 `include/public.hpp` 中的 `msgid` 约定客户端与服务端消息类型，主要包括：

- `LOGIN_MSG` / `LOGIN_MSG_ACK`
- `REG_MSG` / `REG_MSG_ACK`
- `ONE_CHAT_MSG` / `ONE_CHAT_MSG_ACK`
- `ADD_FRIEND_MSG` / `ADD_FRIEND_MSG_ACK`
- `CREATE_GROUP_MSG` / `CREATE_GROUP_MSG_ACK`
- `ADD_GROUP_MSG` / `ADD_GROUP_MSG_ACK`
- `GROUP_CHAT_MSG`
- `LOGINOUT_MSG`

这种设计让网络层不关心业务细节，只负责收发和分发。

## 数据存储设计

从当前代码可以看出，项目至少依赖以下几类数据：

- 用户表：用户 id、用户名、密码、在线状态
- 好友表：好友关系
- 群组表：群组基本信息
- 群成员表：用户加入群组及角色信息
- 离线消息表：用户未读消息持久化

对应代码位置：

- `src/server/model/userModel.cc`
- `src/server/model/friendModel.cc`
- `src/server/model/groupModel.cc`
- `src/server/model/offLineMsgModel.cc`

## 环境依赖

建议在 Linux 环境下运行，至少准备以下依赖：

- `g++` / `clang++`
- `cmake`
- `muduo`
- `mysqlclient` 开发库
- `redis-server`
- `hiredis`
- `nlohmann-json`

如果是 Ubuntu / Debian 系，常见依赖大致如下：

```bash
sudo apt update
sudo apt install cmake g++ libmysqlclient-dev redis-server libhiredis-dev nlohmann-json3-dev
```

`Muduo` 通常需要单独安装或自行编译。

## 配置说明

### 1. MySQL 连接池配置

服务端启动时会读取运行目录下的 `mysql.ini`。当前代码中使用的配置项包括：

```ini
ip=127.0.0.1
port=3306
username=root
password=123456
dbname=chat
initSize=5
maxSize=10
maxIdletime=60
connectionTimeout=1000
```

注意：

- `mysql.ini` 需要放在服务端启动时的工作目录下
- `connectionTimeout` 在当前实现中按代码逻辑读取
- 数据库名、账号密码需要与你本机环境一致

### 2. Redis 配置

当前代码默认连接：

- IP：`127.0.0.1`
- 端口：`6379`
- 密码：空

如需修改，可以从 `src/server/redis/redis.cc` 中调整连接逻辑。

## 构建方式

仓库根目录已提供顶层 `CMakeLists.txt`，推荐直接在项目根目录执行：

```bash
cmake -S . -B build
cmake --build build -j
```

如果只想单独调试某个子目录，也可以按需改用 `src/` 作为入口；但默认构建方式建议以上面的根目录命令为准。

## 运行步骤

### 1. 启动 MySQL 和 Redis

```bash
sudo service mysql start
sudo service redis-server start
```

### 2. 准备数据库

你需要提前创建项目所需的数据表，至少保证以下模块可正常工作：

- 用户表
- 好友关系表
- 群组表
- 群成员表
- 离线消息表

### 3. 启动服务端

```bash
./bin/chat_server 127.0.0.1 6000
```

### 4. 启动客户端

```bash
./bin/chat_client 127.0.0.1 6000
```

### 5. 集群测试

如果要测试集群转发，可启动多个服务端实例，例如：

```bash
./bin/chat_server 127.0.0.1 6000
./bin/chat_server 127.0.0.1 6001
```

让不同用户连接不同端口，再测试单聊或群聊消息是否通过 Redis 转发。

## 客户端命令示例

登录成功后，客户端支持如下命令：

```text
help
chat:friendid:message
groupchat:groupid:message
addfriend:friendid
creategroup:groupname:groupdesc
addgroup:groupid
loginout
```

示例：

```text
addfriend:2
chat:2:hello
creategroup:cpp_group:C++ study group
addgroup:1
groupchat:1:hi everyone
loginout
```

## 关键实现说明

### 1. 网络层与业务层解耦

`ChatServer` 只负责：

- TCP 连接建立与关闭
- 读取客户端 JSON 数据
- 将请求转发给 `IService`

业务逻辑统一在服务层实现，便于扩展更多消息类型。

### 2. 基于 msgid 的路由机制

`IService` 在初始化阶段注册消息处理器，运行时根据 `msgid` 查询回调并执行。这样的好处是：

- 新增业务类型时改动集中
- 网络代码无需关心具体业务
- 结构更接近真实后端网关/服务路由模式

### 3. Redis 发布订阅实现跨节点通信

服务端登录成功后会为用户订阅独立频道；当消息目标用户不在本节点时，当前节点通过 Redis 发布消息，目标节点收到后再转发给对应连接。

### 4. 离线消息处理

如果目标用户离线，消息不会丢失，而是写入离线消息表；用户下次登录时服务端拉取并回传，再清除已投递消息。

### 5. 连接池优化数据库访问

项目实现了简易 MySQL 连接池，支持：

- 初始连接数
- 最大连接数
- 超时等待连接
- 空闲连接回收

## 适合在简历中突出的位置

如果你要把这个项目写到简历里，可以重点强调：

- 基于 `Muduo` 实现高并发 TCP 聊天服务器
- 使用 `Redis Pub/Sub` 实现分布式节点消息转发
- 使用 `MySQL` 持久化用户、好友、群组与离线消息
- 设计 `msgid -> handler` 的业务路由框架，解耦网络层与业务层
- 自定义数据库连接池，降低数据库连接创建开销

可参考的项目描述写法：

```text
基于 Muduo/Redis/MySQL 实现集群聊天服务器，支持注册登录、单聊群聊、好友与群组管理、离线消息存储等功能；通过 Redis 发布订阅机制实现跨节点消息路由，并基于 msgid 设计服务分发框架，完成网络层、业务层和数据层解耦。
```

## 当前可继续优化的方向

这个项目已经具备一个聊天后端的核心雏形，但如果想进一步提升完整度，还可以继续做：

- 补充数据库初始化 SQL 脚本
- 增加统一配置文件，避免 Redis/MySQL 参数硬编码
- 完善异常处理与日志系统
- 增加消息确认、未读计数、心跳机制
- 优化客户端交互体验
- 补充压测、单元测试和接口测试
- 处理 SQL 拼接带来的安全问题，改为预处理语句

## 说明

- 当前仓库里的 `build/`、`bin/` 属于构建产物/可执行文件目录
- `build.sh` 目前为空，可在后续补充一键构建脚本
- `test/testmuduo/` 更偏向 Muduo 学习与验证代码，不属于核心业务模块

## 增量同步

如果你有另一个本地工作目录，可以使用 `scripts/sync-daily.ps1` 把指定文件或目录同步到当前仓库。

1. 示例：
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\scripts\sync-daily.ps1 -Items src include README.md
   ```
2. 如果需要一次完成暂存、提交和推送：
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\scripts\sync-daily.ps1 -Items src include -Stage -CommitMessage "sync part 1" -Push
   ```
