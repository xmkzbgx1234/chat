#include "commonConnectionPool.hpp"
#include "configManager.hpp"
#include "log.h"

using namespace std;

std::string trim(const std::string& s) {
	if (s.empty()) return s;
	// 找到第一个非空白字符（处理开头）
	size_t start = 0;
	while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
		start++;
	}
	// 找到最后一个非空白字符（处理结尾）
	size_t end = s.size();
	while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
		end--;
	}
	return s.substr(start, end - start);
}

// 私有构造函数，防止外部实例化
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool instance; // 编译器保证线程安全的局部静态变量初始化，自动lock和unlock
	return &instance;
}

bool ConnectionPool::loadConfigFile() {
	ConfigManager* config = ConfigManager::getInstance();
	
	if (!config->load("config.ini")) {
		LOG_ERROR << "Failed to load config.ini";
		return false;
	}
	
	// 从配置文件读取数据库配置
	_ip = config->getString("database", "ip", "127.0.0.1");
	_port = config->getInt("database", "port", 3306);
	_username = config->getString("database", "username", "root");
	_password = config->getString("database", "password", "123456");
	_dbname = config->getString("database", "dbname", "chat");
	_initSize = config->getInt("database", "initSize", 5);
	_maxSize = config->getInt("database", "maxSize", 10);
	_maxIdletime = config->getInt("database", "maxIdletime", 60);
	_connectionTimeout = config->getInt("database", "connectionTimeout", 1000);
	
	LOG_INFO << "Database config loaded: " << _ip << ":" << _port << "/" << _dbname;
	return true;
}

ConnectionPool::ConnectionPool() {
	if (!loadConfigFile()) {
		LOG_ERROR << "Failed to load MySQL configuration file";
		return;
	}
	for (int i = 0; i < _initSize; ++i) {
		MySQL* p = new MySQL();
		bool res = p->connect(_ip, _port, _username, _password, _dbname);
		if (!res) {
			LOG_ERROR << "Initial MySQL connection failed";
		}
		p->refreshAliveTime(); //刷新连接的起始空闲时间
		_connectionQue.push(p);
		_connectionCnt++;
	}
	thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
	produce.detach(); // 启动一个独立的线程用于生产连接
	// 启动一个定时线程，扫描超过最大空闲时间_maxIdleTime的连接并销毁
thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
	scanner.detach();
}

void ConnectionPool::returnConnection(MySQL *conn)
{
	unique_lock<mutex> lock(_queueMutex);
	conn->refreshAliveTime();
	_connectionQue.push(conn);
	lock.unlock();
	cv.notify_one();
}
	
void ConnectionPool::produceConnectionTask() {
	while (true) {
		{
			unique_lock<mutex> lock(_queueMutex);
			cv.wait(lock,[this](){return _connectionQue.empty();});
			// 连接数量没有达到上限，继续生产新的连接
			if (_connectionCnt < _maxSize) {
				MySQL* p = new MySQL();
				p->connect(_ip, _port, _username, _password, _dbname);
				p->refreshAliveTime(); //刷新连接的起始空闲时间
				_connectionQue.push(p);
				_connectionCnt++;
			}
		}
		cv.notify_one(); //通知消费者线程可以消费连接了
	}
}

ConnectionPool::ConnectionPtr ConnectionPool::getConnection() {
	auto deleter = [this](MySQL *pconn) {
		if (pconn != nullptr)
		{
			returnConnection(pconn);
		}
	};

	unique_lock<mutex> lock(_queueMutex);
	while(_connectionQue.empty()) {
		if(cv_status::timeout == cv.wait_for(lock, chrono::microseconds(_connectionTimeout))){ //等待连接
			if (_connectionQue.empty()) {
				LOG_WARN << "Get connection timeout";
				return ConnectionPtr(nullptr, deleter); //超时仍然没有连接，返回空指针
			}
		}
	}
	MySQL *conn = _connectionQue.front();
	_connectionQue.pop();
	lock.unlock();
	cv.notify_all(); // 通知生产者线程队列是否为空，以便生产新连接
		
	return ConnectionPtr(conn, deleter);
}

void ConnectionPool::scannerConnectionTask() {
	while (true) {
		this_thread::sleep_for(chrono::seconds(_maxIdletime)); //定时扫描
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize) { //保证连接池数量不小于初始连接数
			MySQL* p = _connectionQue.front();
			if (p->getAliveTime() >= clock_t(_maxIdletime * CLOCKS_PER_SEC)) { //空闲时间超过最大空闲时间
				_connectionQue.pop();
				_connectionCnt--;
				delete p; //销毁连接
			}
			else {
				break; //队头连接没有超过最大空闲时间，后续连接更不会超过，直接跳出循环
			}
		}
	}
}

