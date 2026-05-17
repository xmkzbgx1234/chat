# AGENTS.md - 代码智能体指南

本文档为代码智能体（如华为云码道 CodeArts）提供项目开发指南。

## 项目概述

基于 Muduo + MySQL + Redis + JSON 的 C++ 集群聊天服务器项目，支持用户注册登录、好友聊天、群组聊天、离线消息存储及多服务器节点间消息转发。

**技术栈**：C++17, Muduo, MySQL, Redis (hiredis), nlohmann/json, libsodium, CMake

## 构建命令

### 完整构建
```bash
cmake -S . -B build
cmake --build build -j
```

### Debug 构建（默认）
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

### Release 构建
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### 清理构建
```bash
rm -rf build bin lib
```

### 构建产物
- 可执行文件输出到 `bin/` 目录
- 库文件输出到 `lib/` 目录
- 主要产物：`bin/chat_server`, `bin/chat_client`

## 测试

### 运行所有测试
```bash
# 编译测试文件
cmake -S . -B build
cmake --build build -j

# 运行测试
./bin/test_db_fields
```

### 运行单个测试
```bash
# 直接运行测试可执行文件
./bin/<test_name>
```

### 添加新测试
1. 在 `tests/` 目录创建测试文件，如 `test_xxx.cpp`
2. 在 `CMakeLists.txt` 中添加测试目标
3. 使用 assert 或自定义断言进行验证
4. 返回 0 表示成功，非 0 表示失败

## 代码风格规范

### C++ 标准
- 使用 **C++17** 标准
- 使用现代 C++ 特性（auto, nullptr, range-based for, smart pointers）

### 命名约定
- **类名**：大驼峰（PascalCase），如 `UserModel`, `AuthService`
- **函数名**：小驼峰（camelCase），如 `getUserById`, `handleMessage`
- **成员变量**：下划线前缀，如 `_userModel`, `_msgHandlerMap`
- **常量**：全大写下划线分隔，如 `LOGIN_MSG`, `MAX_CONNECTIONS`
- **枚举**：大驼峰命名，如 `EnMsgType`

### 头文件
- 使用 `#ifndef/#define/#endif` 保护，如 `#ifndef USER_MODEL_HPP`
- 头文件顺序：标准库 → 第三方库 → 项目内部头文件
- 源文件首先包含对应的头文件

### 格式化
- 缩进：4 空格
- 大括号：独占一行（Allman 风格）
- 行宽：建议不超过 120 字符
- 空格：运算符两侧、逗号后、分号后留空格

### 类型使用
- 优先使用 `auto` 进行类型推导
- 使用 `using` 代替 `typedef`
- 智能指针：`std::unique_ptr`, `std::shared_ptr`
- 字符串：`std::string`
- JSON：`nlohmann::json`

### 错误处理
- 使用 Muduo 日志：`LOG_INFO`, `LOG_ERROR`, `LOG_WARN`
- 返回值表示错误：`bool` 或空对象
- 异常：仅在严重错误时使用 `std::runtime_error`
- 错误码：使用 `ErrorCode` 枚举定义统一错误码

### 数据库操作
- **必须**使用预处理语句（prepared statement）防止 SQL 注入
- 示例：
  ```cpp
  const string sql = "INSERT INTO user (name, password) VALUES (?, ?)";
  MYSQL_STMT* stmt = sp->prepare(sql);
  MYSQL_BIND bind[2] = {0};
  // 绑定参数...
  sp->executeStmt(stmt, bind);
  ```
- 使用连接池获取连接：`ConnectionPool::getConnectionPool()->getConnection()`
- 操作完成后关闭语句：`sp->closeStmt(stmt)`

### JSON 处理
- 使用 `nlohmann::json` 库
- 访问字段前检查存在性：`if (js.contains("msgid"))`
- 使用 `Validator` 类进行字段验证
- 响应使用 `ResponseBuilder` 构建统一格式

### 并发安全
- 使用 `std::mutex` 保护共享资源
- 使用 `std::lock_guard` 或 `std::unique_lock` 管理锁
- 在线用户管理通过 `OnlineUserManager` 类统一管理

## 项目结构

```
chat/
├── include/           # 头文件
│   ├── common/       # 通用数据对象
│   ├── server/       # 服务端头文件
│   │   ├── db/       # 数据库封装
│   │   ├── model/    # 数据模型层
│   │   ├── service/  # 业务服务层
│   │   ├── redis/    # Redis 封装
│   │   └── utils/    # 工具类
│   └── qt_client/    # Qt 客户端头文件
├── src/              # 源文件
│   ├── server/       # 服务端实现
│   ├── client/       # 命令行客户端
│   ├── qt_client/    # Qt 图形客户端
│   └── common/       # 通用实体实现
├── tests/            # 测试文件
├── sql/              # SQL 脚本
├── config.ini        # 配置文件
└── CMakeLists.txt    # 构建配置
```

## 消息协议

使用 `msgid` 进行消息路由，定义在 `include/public.hpp`：
- `LOGIN_MSG` (1) / `LOGIN_MSG_ACK` (2)
- `REG_MSG` (3) / `REG_MSG_ACK` (4)
- `ONE_CHAT_MSG` (5) / `ONE_CHAT_MSG_ACK` (6)
- `ADD_FRIEND_MSG` (7) / `ADD_FRIEND_MSG_ACK` (8)
- `CREATE_GROUP_MSG` (9) / `CREATE_GROUP_MSG_ACK` (10)
- `ADD_GROUP_MSG` (11) / `ADD_GROUP_MSG_ACK` (12)
- `GROUP_CHAT_MSG` (13)
- `LOGINOUT_MSG` (15)
- `KICK_OFF_MSG` (16)

## 配置文件

`config.ini` 包含以下配置节：
- `[database]` - MySQL 连接配置
- `[server]` - 服务器配置
- `[redis]` - Redis 连接配置
- `[security]` - 安全配置（密码长度、消息长度限制）
- `[log]` - 日志配置

## 运行服务

### 启动依赖服务
```bash
sudo service mysql start
sudo service redis-server start
```

### 启动服务器
```bash
./bin/chat_server 127.0.0.1 6000
```

### 启动客户端
```bash
./bin/chat_client 127.0.0.1 6000
```

## 依赖库

项目依赖以下库（通过 CMake 链接）：
- `muduo_net` - Muduo 网络库
- `muduo_base` - Muduo 基础库
- `mysqlclient` - MySQL 客户端库
- `hiredis` - Redis C 客户端库
- `pthread` - POSIX 线程库
- `sodium` - libsodium 加密库

## 开发注意事项

1. **安全性**：所有 SQL 操作必须使用预处理语句，禁止字符串拼接 SQL
2. **资源管理**：使用 RAII，确保资源正确释放
3. **日志记录**：关键操作添加日志，便于调试和监控
4. **参数验证**：使用 `Validator` 类验证所有外部输入
5. **单例模式**：服务类使用单例模式，通过 `instance()` 方法获取实例
6. **消息编码**：使用 `encodeMessage()` 和 `tryDecodeMessages()` 处理 TCP 粘包

## 常见开发任务

### 添加新消息类型
1. 在 `public.hpp` 的 `EnMsgType` 枚举中添加消息 ID
2. 在对应 Service 中实现处理函数
3. 在 `IService` 构造函数中注册消息处理器
4. 更新客户端协议处理逻辑

### 添加新服务类
1. 在 `include/server/service/` 创建头文件
2. 在 `src/server/service/` 创建实现文件
3. 继承或参考 `AuthService` 的实现模式
4. 在 `IService` 中添加成员变量并初始化

### 添加数据模型
1. 在 `include/server/model/` 创建头文件
2. 在 `src/server/model/` 创建实现文件
3. 参照 `UserModel` 使用预处理语句
4. 在 `IService` 中添加成员变量

## WSL 环境构建（如适用）

如果在 WSL 中构建但使用 Windows 工具链：
```bash
# 使用 CMake + Make
cmake -S . -B build-wsl
cmake --build build-wsl -j
```

## 注意事项

- 默认构建类型为 Debug，包含调试符号（`-g`）
- Release 构建启用优化（`-O3 -DNDEBUG`）
- 构建前确保所有依赖库已安装
- 配置文件 `config.ini` 需放在工作目录
