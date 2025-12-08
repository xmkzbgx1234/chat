#ifndef GROUP_HPP
#define GROUP_HPP

#include <string>
#include <vector>
#include "groupUser.hpp"

using namespace std;


class Group
{
public:
    Group() = default;
    Group(int id, const string &name, const string &desc): id(id), name(name), desc(desc){};
    ~Group() = default;
    void setId(int id){
        this->id = id;
    }
    int getId() const{
        return this->id;
    }
    void setName(const string &name){
        this->name = name;
    }
    string getName() const{
        return this->name;
    }
    void setDesc(const string &desc){
        this->desc = desc;
    }
    string getDesc() const{
        return this->desc;
    }
    void setUsers(const vector<groupUser> &users){
        this->users = users;
    }
    vector<groupUser>& getUsers(){
        return this->users;
    }
protected:
    int id;
    string name;
    string desc;
    vector<groupUser> users;
};


#endif // GROUP_HPP