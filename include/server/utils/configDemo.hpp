#ifndef CONFIG_DEMO_HPP
#define CONFIG_DEMO_HPP

#include "configManager.hpp"
#include "log.h"

// 配置使用示例
class ConfigDemo {
public:
    static void showExample() {
        ConfigManager* config = ConfigManager::getInstance();
        
        // 获取数据库配置
        string dbIp = config->getString("database", "ip", "127.0.0.1");
        int dbPort = config->getInt("database", "port", 3306);
        LOG_INFO << "Database: " << dbIp << ":" << dbPort;
        
        // 获取服务器配置
        string serverHost = config->getString("server", "host", "0.0.0.0");
        int serverPort = config->getInt("server", "port", 6000);
        LOG_INFO << "Server: " << serverHost << ":" << serverPort;
        
        // 获取安全配置
        int pwdMinLen = config->getInt("security", "passwordMinLength", 6);
        bool enableKickOff = config->getBool("security", "enableKickOff", true);
        LOG_INFO << "Security: pwdMinLen=" << pwdMinLen << ", kickOff=" << enableKickOff;
        
        // 获取功能开关
        bool enableOffline = config->getBool("features", "enableOfflineMessage", true);
        bool enableGroup = config->getBool("features", "enableGroupChat", true);
        LOG_INFO << "Features: offline=" << enableOffline << ", group=" << enableGroup;
    }
};

#endif // CONFIG_DEMO_HPP
