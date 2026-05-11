#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

class ConfigManager {
private:
    std::map<std::string, std::map<std::string, std::string>> _config;
    std::string _configFile;
    
    ConfigManager() {}
    
    std::string trim(const std::string& str);

public:
    static ConfigManager* getInstance();
    
    bool load(const std::string& configFile);
    
    std::string getString(const std::string& section, const std::string& key, const std::string& defaultValue = "");
    
    int getInt(const std::string& section, const std::string& key, int defaultValue = 0);
    
    bool getBool(const std::string& section, const std::string& key, bool defaultValue = false);
    
    double getDouble(const std::string& section, const std::string& key, double defaultValue = 0.0);
    
    void set(const std::string& section, const std::string& key, const std::string& value);
    
    bool save();
    
    bool reload();
    
    void print();
};

#endif // CONFIG_MANAGER_HPP
