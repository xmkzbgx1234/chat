#ifndef DB_HPP
#define DB_HPP

#include <mysql/mysql.h>
#include <string>
#include <ctime>
#include <vector>

class MySQL
{
public:
	MySQL();
	~MySQL();
	bool connect(std::string ip, unsigned short port, std::string user, std::string password, std::string dbname);
	bool update(std::string sql);
	MYSQL_RES* query(std::string sql);
	std::string escapeString(const std::string &input);

	// 预编译语句支持
	MYSQL_STMT* prepare(const std::string& sql);
	bool executeStmt(MYSQL_STMT* stmt, MYSQL_BIND* bind);
	MYSQL_RES* queryStmt(MYSQL_STMT* stmt, MYSQL_BIND* bind);
	void closeStmt(MYSQL_STMT* stmt);

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
