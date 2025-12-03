#ifndef USER_HPP
#define USER_HPP

#include <string>

class User
{
public:
    User();
    User(int id, const std::string &name, const std::string &password, const std::string &state);

    int getId() const;
    void setId(int uid);

    std::string getName() const;
    void setName(const std::string &uname);

    std::string getPassword() const;
    void setPassword(const std::string &pwd);

    void setState(const std::string &s);
    std::string getState() const;
    
private:
    int id;
    std::string name;
    std::string password;
    std::string state; // 用户状态：在线、离线
};

#endif // USER_HPP