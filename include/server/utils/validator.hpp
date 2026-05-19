#ifndef VALIDATOR_HPP
#define VALIDATOR_HPP

#include <string>
#include <nlohmann/json.hpp>

class Validator {
public:
    // 字符串非空校验
    static bool isNotEmpty(const std::string& str) {
        return !str.empty();
    }
    
    // 用户ID有效校验
    static bool isValidUserId(int userid) {
        return userid > 0;
    }
    
    // 群组ID有效校验
    static bool isValidGroupId(int groupid) {
        return groupid > 0;
    }
    
    // 消息内容有效校验
    static bool isValidMessage(const std::string& msg) {
        return !msg.empty() && msg.size() <= 6000;  // 最大6000字符
    }
    
    // 密码强度校验
    static bool isValidPassword(const std::string& password) {
        return password.size() >= 6 && password.size() <= 128;
    }
    
    // 用户名校验
    static bool isValidUsername(const std::string& username) {
        return username.size() >= 2 && username.size() <= 64;
    }
    
    // 群名校验
    static bool isValidGroupName(const std::string& groupname) {
        return !groupname.empty() && groupname.size() <= 128;
    }
    
    // JSON字段存在校验
    static bool hasField(const nlohmann::json& js, const std::string& field) {
        return js.contains(field) && !js[field].is_null();
    }
    
    // 获取JSON字符串字段（带默认值）
    static std::string getString(const nlohmann::json& js, const std::string& field, const std::string& defaultValue = "") {
        if (hasField(js, field) && js[field].is_string()) {
            return js[field].get<std::string>();
        }
        return defaultValue;
    }
    
    // 获取JSON整数字段（带默认值）
    static int getInt(const nlohmann::json& js, const std::string& field, int defaultValue = 0) {
        if (hasField(js, field) && js[field].is_number_integer()) {
            return js[field].get<int>();
        }
        return defaultValue;
    }
};

#endif // VALIDATOR_HPP
