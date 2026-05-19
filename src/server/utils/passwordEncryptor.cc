#include "passwordEncryptor.hpp"
#include <vector>
#include <stdexcept>
#include "log.h"

PasswordEncryptor::PasswordEncryptor() {
    if (sodium_init() < 0) {
        LOG_ERROR << "libsodium initialization failed";
    }
}

PasswordEncryptor& PasswordEncryptor::getInstance(){
    static PasswordEncryptor inst;
    return inst;
}

std::string PasswordEncryptor::hashPassword(const std::string& password) {
    char hash[crypto_pwhash_STRBYTES];
    if(crypto_pwhash_str(hash, password.c_str(), password.size(),
                       crypto_pwhash_OPSLIMIT_INTERACTIVE,
                       crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0){
        LOG_ERROR << "Password hashing failed";
        throw std::runtime_error("Password hashing failed");
    }
    return std::string(hash);  // 自动在 '\0' 处终止
}

bool PasswordEncryptor::verifyPassword(const std::string& password, 
                                     const std::string& hash) {
    if(hash.empty()){
        LOG_ERROR << "Hash is empty";
        return false;
    }
    return crypto_pwhash_str_verify(hash.c_str(), password.c_str(), password.size()) == 0;
}