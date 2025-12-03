#include "User.hpp"

User::User() : id(-1), name(""), password(""), state("offline") {}
User::User(int id, const std::string &name, const std::string &password, const std::string &state)
    : id(id), name(name), password(password), state(state) {}
int User::getId() const { return this->id; }
void User::setId(int uid) { this->id = uid; }

std::string User::getName() const { return this->name; }
void User::setName(const std::string &uname) { this->name = uname; }

std::string User::getPassword() const { return this->password; }
void User::setPassword(const std::string &pwd) { this->password = pwd; }

void User::setState(const std::string &s) { this->state = s; }
std::string User::getState() const { return this->state; }