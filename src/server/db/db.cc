#include "db.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>

using namespace std;

MySQL::MySQL()
{
	_conn = mysql_init(nullptr);
	if (_conn == nullptr) {
		LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << "mysql_init failed";
	}
	else {
		LOG_INFO << "mysql_init success";
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
	MYSQL *p = mysql_real_connect(_conn, ip.c_str(), user.c_str(),
		password.c_str(), dbname.c_str(), port, nullptr, 0);
	if (p == nullptr) {
		LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << "mysql_real_connect failed:";
		return false;
	}
	else {
		LOG_INFO << "mysql_real_connect success";
	}
	// C和C++的默认编码是ASCII，而MySQL的默认编码是latin1，因此在连接上MySQL后需要设置编码，否则可能出现中文乱码的问题
	mysql_query(_conn, "SET NAMES gbk");
	return true;
}

bool MySQL::update(string sql)
{
	if (mysql_query(_conn, sql.c_str())) {
		LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "mysql_query failed";
		return false;
	}
	else {
		LOG_INFO << "mysql_query success";
	}
	return true;
}

MYSQL_RES* MySQL::query(string sql)
{
	if (mysql_query(_conn, sql.c_str())) {
		LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "mysql_query failed:";
		return nullptr;
	}
	else {
		LOG_INFO << "mysql_query success";
	}
	return mysql_store_result(_conn);
}

string MySQL::escapeString(const string &input)
{
	if (_conn == nullptr) {
		return input;
	}
	string escaped;
	escaped.resize(input.size() * 2 + 1);
	unsigned long len = mysql_real_escape_string(_conn, &escaped[0], input.c_str(),
		static_cast<unsigned long>(input.size()));
	escaped.resize(len);
	return escaped;
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
