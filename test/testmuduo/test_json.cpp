#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
using namespace std; 

using json = nlohmann::json;

void func1(){
    json data;
    data["msg_type"] = 2;
    data["from"] = "source";
    data["to"] = "destination";
    data["msg"] = "Hello, World!";
    std::cout << data << std::endl;
    string sendBuf = data.dump();
    cout << sendBuf.c_str() << endl;

}
int main() {
    json root;
    root["name"] = "Ubuntu"; 
    root["version"] = 22.04;
    std::cout << root << std::endl;
    func1();
    return 0;
}