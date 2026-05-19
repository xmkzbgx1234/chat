#include "db.hpp"
#include "public.hpp"
#include "log.h"

using namespace std;

MySQL::MySQL()
{
	_conn = mysql_init(nullptr);
	if (_conn == nullptr) {
		LOG_ERROR << "mysql_init failed";
	}
	else {
		LOG_DEBUG << "mysql_init success";
	}
}

MySQL::~MySQL()
{
	if (_conn != nullptr) {
		mysql_close(_conn);
	}
}

bool MySQL::connect(string ip,
	unsigned short port,
	string user,
	string password,
	string dbname)
{
	_ip = ip;
	_port = port;
	_user = user;
	_password = password;
	_dbname = dbname;

	// 启用自动重连
	bool reconnect = 1;
	mysql_options(_conn, MYSQL_OPT_RECONNECT, &reconnect);

	MYSQL *p = mysql_real_connect(_conn, ip.c_str(), user.c_str(),
		password.c_str(), dbname.c_str(), port, nullptr, 0);
	if (p == nullptr) {
		LOG_ERROR << "mysql_real_connect failed";
		return false;
	}
	else {
		LOG_DEBUG << "mysql_real_connect success";
	}
	mysql_query(_conn, "SET NAMES gbk");
	return true;
}

bool MySQL::ping()
{
	if (_conn == nullptr) return false;
	if (mysql_ping(_conn) == 0) return true;
	// ping失败，尝试重连
	LOG_WARN << "MySQL ping failed, attempting reconnect";
	mysql_close(_conn);
	_conn = mysql_init(nullptr);
	if (_conn == nullptr) return false;
	bool reconnect = 1;
	mysql_options(_conn, MYSQL_OPT_RECONNECT, &reconnect);
	MYSQL *p = mysql_real_connect(_conn, _ip.c_str(), _user.c_str(),
		_password.c_str(), _dbname.c_str(), _port, nullptr, 0);
	if (p == nullptr)
	{
		LOG_ERROR << "MySQL reconnect failed";
		return false;
	}
	mysql_query(_conn, "SET NAMES gbk");
	LOG_INFO << "MySQL reconnect success";
	return true;
}

bool MySQL::update(string sql)
{
	if (mysql_query(_conn, sql.c_str())) {
		LOG_ERROR << sql << " mysql_query failed";
		return false;
	}
	else {
		LOG_DEBUG << "mysql_query success";
	}
	return true;
}

MYSQL_RES* MySQL::query(string sql)
{
	if (mysql_query(_conn, sql.c_str())) {
		LOG_ERROR << sql << " mysql_query failed";
		return nullptr;
	}
	else {
		LOG_DEBUG << "mysql_query success";
	}
	return mysql_store_result(_conn);
}

MYSQL* MySQL::getMySQL()
{
	return _conn;
}

MYSQL_STMT* MySQL::prepare(const string& sql)
{
	if (_conn == nullptr) {
		LOG_ERROR << "MySQL connection is null";
		return nullptr;
	}

	MYSQL_STMT* stmt = mysql_stmt_init(_conn);
	if (stmt == nullptr) {
		LOG_ERROR << "mysql_stmt_init failed";
		return nullptr;
	}

	if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length()) != 0) {
		LOG_ERROR << "mysql_stmt_prepare failed: " << mysql_stmt_error(stmt);
		mysql_stmt_close(stmt);
		return nullptr;
	}

	return stmt;
}

bool MySQL::executeStmt(MYSQL_STMT* stmt, MYSQL_BIND* bind)
{
	if (stmt == nullptr) {
		LOG_ERROR << "Statement is null";
		return false;
	}

	if (bind != nullptr && mysql_stmt_bind_param(stmt, bind) != 0) {
		LOG_ERROR << "mysql_stmt_bind_param failed: " << mysql_stmt_error(stmt);
		return false;
	}

	if (mysql_stmt_execute(stmt) != 0) {
		LOG_ERROR << "mysql_stmt_execute failed: " << mysql_stmt_error(stmt);
		return false;
	}

	return true;
}

MYSQL_RES* MySQL::queryStmt(MYSQL_STMT* stmt, MYSQL_BIND* bind)
{
	if (stmt == nullptr) {
		LOG_ERROR << "Statement is null";
		return nullptr;
	}

	if (bind != nullptr && mysql_stmt_bind_param(stmt, bind) != 0) {
		LOG_ERROR << "mysql_stmt_bind_param failed: " << mysql_stmt_error(stmt);
		return nullptr;
	}

	if (mysql_stmt_execute(stmt) != 0) {
		LOG_ERROR << "mysql_stmt_execute failed: " << mysql_stmt_error(stmt);
		return nullptr;
	}

	return mysql_stmt_result_metadata(stmt);
}

void MySQL::closeStmt(MYSQL_STMT* stmt)
{
	if (stmt != nullptr) {
		mysql_stmt_close(stmt);
	}
}
