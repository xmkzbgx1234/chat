#include "chatServer.hpp"
#include "IService.hpp"
#include "configManager.hpp"
#include "log.h"
#include <signal.h>
#include <string>
#include <thread>
#include <chrono>

using namespace std;

void resetHandler(int sig)
{
    LOG_INFO << "Receive signal " << sig << ", reset user state before exit";
    IService::instance()->reset();
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, resetHandler);
    
    // 加载配置文件
    ConfigManager* config = ConfigManager::getInstance();
    string configFile = "config.ini";
    
    // 支持命令行指定配置文件
    if (argc >= 2) {
        configFile = argv[1];
    }
    
    if (!config->load(configFile)) {
        LOG_ERROR << "Failed to load config file: " << configFile;
        return -1;
    }
    
    LOG_INFO << "Configuration loaded from: " << configFile;
    
    // 从配置文件读取服务器配置
    string ip = config->getString("server", "host", "0.0.0.0");
    int port = config->getInt("server", "port", 6000);
    
    // 支持命令行参数覆盖配置文件
    if (argc >= 4) {
        ip = argv[2];
        port = atoi(argv[3]);
    }
    
    EventLoop loop;
    InetAddress addr(ip, port);
    IService::instance()->reset();
    ChatServer server(&loop, addr, "ChatServer");

    LOG_INFO << "Chat server starting on " << ip << ":" << port;
    server.start();

    // 等待 worker 线程事件循环完全就绪，避免第一个连接因线程未准备好而丢包
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    loop.loop();

    return 0;
}
