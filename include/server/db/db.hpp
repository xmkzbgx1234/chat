#ifndef DB_HPP
#define DB_HPP

#include <mysql/mysql.h>
#include <string>
#include <ctime>

static std::string server = "127.0.0.1";
static std::string user = "root";
static std::string password = "123456";
static std::string dbname = "chat";

class MySQL
{
public:
	MySQL();
	~MySQL();
	bool connect();
	bool update(std::string sql);
	MYSQL_RES* query(std::string sql);

	void refreshAliveTime() { // 刷新连接的起始空闲时间
		_aliveTime = clock();
	} 
	clock_t getAliveTime() const { // 获取连接的空闲时间
		return clock() - _aliveTime;
	} 

	// 获取当前连接的MYSQL指针
	MYSQL* getMySQL();
private:
	MYSQL* _conn;
	clock_t _aliveTime; // 连接的空闲时间
};

#endif // DB_HPP