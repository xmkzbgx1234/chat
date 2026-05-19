#include "configManager.hpp"
#include <iostream>

std::string ConfigManager::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

ConfigManager* ConfigManager::getInstance() {
    static ConfigManager instance;
    return &instance;
}

bool ConfigManager::load(const std::string& configFile) {
    _configFile = configFile;
    std::ifstream ifs(configFile);
    if (!ifs.is_open()) {
        return false;
    }
    
    std::string line;
    std::string currentSection = "default";
    
    while (std::getline(ifs, line)) {
        line = trim(line);
        
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }
        
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));
            _config[currentSection][key] = value;
        }
    }
    
    ifs.close();
    return true;
}

std::string ConfigManager::getString(const std::string& section, const std::string& key, const std::string& defaultValue) {
    auto sectionIt = _config.find(section);
    if (sectionIt != _config.end()) {
        auto keyIt = sectionIt->second.find(key);
        if (keyIt != sectionIt->second.end()) {
            return keyIt->second;
        }
    }
    return defaultValue;
}

int ConfigManager::getInt(const std::string& section, const std::string& key, int defaultValue) {
    std::string value = getString(section, key, "");
    if (!value.empty()) {
        try {
            return std::stoi(value);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool ConfigManager::getBool(const std::string& section, const std::string& key, bool defaultValue) {
    std::string value = getString(section, key, "");
    if (!value.empty()) {
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        return (value == "true" || value == "1" || value == "yes");
    }
    return defaultValue;
}

double ConfigManager::getDouble(const std::string& section, const std::string& key, double defaultValue) {
    std::string value = getString(section, key, "");
    if (!value.empty()) {
        try {
            return std::stod(value);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

void ConfigManager::set(const std::string& section, const std::string& key, const std::string& value) {
    _config[section][key] = value;
}

bool ConfigManager::save() {
    std::ofstream ofs(_configFile);
    if (!ofs.is_open()) {
        return false;
    }
    
    for (const auto& section : _config) {
        ofs << "[" << section.first << "]" << std::endl;
        for (const auto& kv : section.second) {
            ofs << kv.first << "=" << kv.second << std::endl;
        }
        ofs << std::endl;
    }
    
    ofs.close();
    return true;
}

bool ConfigManager::reload() {
    return load(_configFile);
}

void ConfigManager::print() {
    for (const auto& section : _config) {
        std::cout << "[" << section.first << "]" << std::endl;
        for (const auto& kv : section.second) {
            std::cout << "  " << kv.first << "=" << kv.second << std::endl;
        }
    }
}
