#ifndef ERROR_CODE_HPP
#define ERROR_CODE_HPP

#include <string>
#include <unordered_map>

// 统一错误码定义
enum class ErrorCode {
    // 成功
    SUCCESS = 0,
    
    // 通用错误 (1-99)
    UNKNOWN_ERROR = 1,
    INVALID_PARAMS = 2,
    INVALID_USER_ID = 3,
    INVALID_PASSWORD = 4,
    INVALID_USERNAME = 5,
    INVALID_GROUP_ID = 6,
    INVALID_MESSAGE = 7,
    
    // 认证相关错误 (100-199)
    AUTH_LOGIN_FAILED = 100,
    AUTH_USER_NOT_FOUND = 101,
    AUTH_PASSWORD_WRONG = 102,
    AUTH_USER_ALREADY_ONLINE = 103,
    AUTH_REGISTER_FAILED = 104,
    AUTH_USERNAME_EXISTS = 105,
    
    // 好友相关错误 (200-299)
    FRIEND_ADD_FAILED = 200,
    FRIEND_ALREADY_EXISTS = 201,
    FRIEND_NOT_FOUND = 202,
    FRIEND_CANNOT_ADD_SELF = 203,
    FRIEND_USER_NOT_EXIST = 204,
    
    // 群组相关错误 (300-399)
    GROUP_CREATE_FAILED = 300,
    GROUP_JOIN_FAILED = 301,
    GROUP_NOT_FOUND = 302,
    GROUP_ALREADY_MEMBER = 303,
    GROUP_NAME_INVALID = 304,
    
    // 消息相关错误 (400-499)
    MSG_SEND_FAILED = 400,
    MSG_TARGET_NOT_FOUND = 401,
    MSG_PERSISTENCE_FAILED = 402,
    MSG_CONTENT_INVALID = 403,
    
    // 数据库相关错误 (500-599)
    DB_ERROR = 500,
    DB_INSERT_FAILED = 501,
    DB_QUERY_FAILED = 502,
    DB_UPDATE_FAILED = 503,
    
    // 网络相关错误 (600-699)
    NET_DISCONNECTED = 600,
    NET_TIMEOUT = 601,
    NET_CONNECTION_ERROR = 602
};

// 错误码管理类
class ErrorManager {
private:
    static std::unordered_map<ErrorCode, std::string> _errorMessages;
    
public:
    // 获取错误消息
    static std::string getMessage(ErrorCode code) {
        auto it = _errorMessages.find(code);
        if (it != _errorMessages.end()) {
            return it->second;
        }
        return "未知错误";
    }
    
    // 判断是否成功
    static bool isSuccess(ErrorCode code) {
        return code == ErrorCode::SUCCESS;
    }
    
    // 判断是否为错误
    static bool isError(ErrorCode code) {
        return code != ErrorCode::SUCCESS;
    }
};

// 错误消息映射（在errorCode.cpp中实现）
inline std::unordered_map<ErrorCode, std::string> ErrorManager::_errorMessages = {
    // 成功
    {ErrorCode::SUCCESS, "操作成功"},
    
    // 通用错误
    {ErrorCode::UNKNOWN_ERROR, "未知错误"},
    {ErrorCode::INVALID_PARAMS, "参数无效"},
    {ErrorCode::INVALID_USER_ID, "无效的用户ID"},
    {ErrorCode::INVALID_PASSWORD, "密码格式无效"},
    {ErrorCode::INVALID_USERNAME, "用户名格式无效"},
    {ErrorCode::INVALID_GROUP_ID, "无效的群组ID"},
    {ErrorCode::INVALID_MESSAGE, "消息内容无效"},
    
    // 认证错误
    {ErrorCode::AUTH_LOGIN_FAILED, "登录失败"},
    {ErrorCode::AUTH_USER_NOT_FOUND, "用户不存在"},
    {ErrorCode::AUTH_PASSWORD_WRONG, "密码错误"},
    {ErrorCode::AUTH_USER_ALREADY_ONLINE, "用户已在线"},
    {ErrorCode::AUTH_REGISTER_FAILED, "注册失败"},
    {ErrorCode::AUTH_USERNAME_EXISTS, "用户名已存在"},
    
    // 好友错误
    {ErrorCode::FRIEND_ADD_FAILED, "添加好友失败"},
    {ErrorCode::FRIEND_ALREADY_EXISTS, "已是好友"},
    {ErrorCode::FRIEND_NOT_FOUND, "好友不存在"},
    {ErrorCode::FRIEND_CANNOT_ADD_SELF, "不能添加自己为好友"},
    {ErrorCode::FRIEND_USER_NOT_EXIST, "目标用户不存在"},
    
    // 群组错误
    {ErrorCode::GROUP_CREATE_FAILED, "创建群组失败"},
    {ErrorCode::GROUP_JOIN_FAILED, "加入群组失败"},
    {ErrorCode::GROUP_NOT_FOUND, "群组不存在"},
    {ErrorCode::GROUP_ALREADY_MEMBER, "已在群组中"},
    {ErrorCode::GROUP_NAME_INVALID, "群名无效"},
    
    // 消息错误
    {ErrorCode::MSG_SEND_FAILED, "消息发送失败"},
    {ErrorCode::MSG_TARGET_NOT_FOUND, "目标用户不存在"},
    {ErrorCode::MSG_PERSISTENCE_FAILED, "消息持久化失败"},
    {ErrorCode::MSG_CONTENT_INVALID, "消息内容无效"},
    
    // 数据库错误
    {ErrorCode::DB_ERROR, "数据库错误"},
    {ErrorCode::DB_INSERT_FAILED, "数据库插入失败"},
    {ErrorCode::DB_QUERY_FAILED, "数据库查询失败"},
    {ErrorCode::DB_UPDATE_FAILED, "数据库更新失败"},
    
    // 网络错误
    {ErrorCode::NET_DISCONNECTED, "连接已断开"},
    {ErrorCode::NET_TIMEOUT, "操作超时"},
    {ErrorCode::NET_CONNECTION_ERROR, "连接错误"}
};

#endif // ERROR_CODE_HPP
