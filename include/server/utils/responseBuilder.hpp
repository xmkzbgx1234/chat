#ifndef RESPONSE_BUILDER_HPP
#define RESPONSE_BUILDER_HPP

#include <nlohmann/json.hpp>
#include "errorCode.hpp"
#include "public.hpp"

using json = nlohmann::json;

class ResponseBuilder {
public:
    // 构建成功响应
    static json success(int msgid, const std::string& message = "操作成功") {
        json response;
        response["msgid"] = msgid;
        response["errno"] = static_cast<int>(ErrorCode::SUCCESS);
        response["errmsg"] = message;
        return response;
    }
    
    // 构建错误响应
    static json error(int msgid, ErrorCode code) {
        json response;
        response["msgid"] = msgid;
        response["errno"] = static_cast<int>(code);
        response["errmsg"] = ErrorManager::getMessage(code);
        return response;
    }
    
    // 构建错误响应（自定义消息）
    static json error(int msgid, ErrorCode code, const std::string& customMessage) {
        json response;
        response["msgid"] = msgid;
        response["errno"] = static_cast<int>(code);
        response["errmsg"] = customMessage;
        return response;
    }
    
    // 构建响应（泛型）
    static json build(int msgid, ErrorCode code, const json& data = json()) {
        json response;
        response["msgid"] = msgid;
        response["errno"] = static_cast<int>(code);
        response["errmsg"] = ErrorManager::getMessage(code);
        if (!data.empty()) {
            for (auto it = data.begin(); it != data.end(); ++it) {
                response[it.key()] = it.value();
            }
        }
        return response;
    }
};

#endif // RESPONSE_BUILDER_HPP
