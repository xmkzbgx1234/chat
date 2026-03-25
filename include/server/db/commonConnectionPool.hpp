#ifndef COMMONCONNECTIONPOOL_HPP
#define COMMONCONNECTIONPOOL_HPP
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <thread>
#include <memory>
#include <functional>
#include "db.hpp"

class ConnectionPool
{
public:
	// 获取单例的连接池实例
	static ConnectionPool* getConnectionPool();
	// 外部申请连接的接口，智能指针在用户使用完连接后自动归还到连接池
	std::shared_ptr<MySQL> getConnection(); 

	bool loadConfigFile(); // 加载配置文件
	// 运行在独立的线程，用于生产新连接
	void produceConnectionTask();
	void scannerConnectionTask();
private:
	std::string _ip; // mysql IP地址
	unsigned int _port; // mysql端口号
	std::string _username; // 用户名
	std::string _password; // 密码
	std::string _dbname; // 数据库名
	unsigned int _initSize; // 连接池初始连接数
	unsigned int _maxSize; // 连接池最大连接数
	unsigned int _maxIdletime; // 连接最大空闲时间
	int _connectionTimeout; // 连接超时时间
	std::atomic_int _connectionCnt;

	std::queue<MySQL*> _connectionQue; // 存储mysql连接的队列
	std::mutex _queueMutex; // 保护连接队列的线程安全互斥锁

	std::condition_variable cv; //条件变量，用于生产线程和消费线程之间的通信
	ConnectionPool();

};

#endif

