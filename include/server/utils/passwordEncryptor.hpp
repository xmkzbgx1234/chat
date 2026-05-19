// passwordEncryptor.hpp
#ifndef PASSWORDENCRYPTO_HPP
#define PASSWORDENCRYPTOR_HPP

#include <string>
#include <sodium.h>

class PasswordEncryptor {
private:
    PasswordEncryptor();
    
public:
    static PasswordEncryptor& getInstance();
    
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);
    
    // 禁止拷贝
    PasswordEncryptor(const PasswordEncryptor&) = delete;
    PasswordEncryptor& operator=(const PasswordEncryptor&) = delete;
};

#endif