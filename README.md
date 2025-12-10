# chat

#### 介绍
这是一个基于 C++ 的聊天服务器项目，使用了 Muduo 网络库和 MySQL 数据库，实现了用户登录、注册、添加好友、创建和加入群组等功能。

#### 软件架构
该项目采用经典的客户端-服务器架构，主要包括以下几个模块：
- **用户模块**：处理用户登录、注册和信息更新。
- **好友模块**：处理添加好友和查询好友信息。
- **群组模块**：支持创建群组、加入群组以及查询群组成员信息。
- **数据库模块**：提供与 MySQL 数据库的交互，包括数据的增删改查。

#### 安装教程
1. 安装依赖库：Muduo 网络库、MySQL 数据库、nlohmann/json。
2. 克隆项目到本地：
   ```bash
   git clone https://gitee.com/xu-mankun/chat.git
   ```
3. 编译项目：
   ```bash
   cd chat
   mkdir build && cd build
   cmake ..
   make
   ```

#### 使用说明
1. 启动聊天服务器：
   ```bash
   ./chat_server
   ```
2. 使用客户端连接服务器并进行登录、注册、添加好友、创建和加入群组等操作。

#### 参与贡献
1. Fork 本仓库。
2. 新建 Feat_xxx 分支。
3. 提交代码。
4. 新建 Pull Request。

#### 特技
1. 使用 `Readme_XXX.md` 来支持不同的语言，例如 `Readme_en.md`, `Readme_zh.md`。
2. Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)。
3. 你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目。
4. [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目。
5. Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)。
6. Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)。