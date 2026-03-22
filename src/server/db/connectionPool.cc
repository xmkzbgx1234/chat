#include "commonConnectionPool.hpp"

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
	// 使用 C++ 的 ifstream 打开文件（替代 FILE*）
	std::ifstream ifs("mysql.ini");
	if (!ifs.is_open()) { // 检查文件是否打开成功
		cerr << "mysql.ini open failed!";
		return false;
	}

	std::string line;
	// 使用 getline 逐行读取（替代 fgets），自动处理换行符，无固定缓冲区限制
	while (std::getline(ifs, line)) {
		// 处理注释行（以 # 开头）和空行
		std::string trimmedLine = trim(line);
		if (trimmedLine.empty() || trimmedLine[0] == '#') {
			continue;
		}

		// 分割键值对（寻找 '=' 位置）
		size_t pos = line.find('=');
		if (pos == std::string::npos) { // 无 '=' 符号，跳过无效行
			continue;
		}

		// 提取键和值，并修剪空白字符
		std::string key = trim(line.substr(0, pos));
		std::string value = trim(line.substr(pos + 1));

		// 赋值（对数值类型使用 try-catch 避免转换失败崩溃）
		try {
			if (key == "ip") {
				_ip = value;
			}
			else if (key == "port") {
				_port = std::stoi(value); // 字符串转整数
			}
			else if (key == "username") {
				_username = value;
			}
			else if (key == "password") {
				_password = value;
			}
			else if (key == "dbname") {
				_dbname = value;
			}
			else if (key == "initSize") {
				_initSize = std::stoi(value);
			}
			else if (key == "maxSize") {
				_maxSize = std::stoi(value);
			}
			else if (key == "maxIdletime") {
				_maxIdletime = std::stoi(value);
			}
			else if (key == "connectionTimeout") {
				_connectionTimeout = std::stoi(value);
			}
		}
		catch (const std::exception& e) {
			cerr << "Parse config error! key: " + key + ", value: " + value + ", error: " + e.what();
			ifs.close(); // 关闭文件
			return false;
		}
	}

	// 检查文件是否正常读取结束（非错误导致的中断）
	if (ifs.bad()) {
		cerr << "Error reading mysql.ini!";
		ifs.close();
		return false;
	}

	ifs.close(); // 显式关闭文件（ifstream 析构时也会关闭，显式关闭更清晰）
	return true;
}

ConnectionPool::ConnectionPool() {
	if (!loadConfigFile()) {
		cerr << "Failed to load MySQL configuration file.";
		return;
	}
	for (int i = 0; i < _initSize; ++i) {
		MySQL* p = new MySQL();
		bool res = p->connect(_ip, _port, _username, _password, _dbname);
		if (!res) cerr << "connect error!";
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

shared_ptr<MySQL> ConnectionPool::getConnection() {
	unique_lock<mutex> lock(_queueMutex);
	while(_connectionQue.empty()) {
		if(cv_status::timeout == cv.wait_for(lock, chrono::microseconds(_connectionTimeout))){ //等待连接
			if (_connectionQue.empty()) {
				cerr << "Get connection timeout!";
				return nullptr; //超时仍然没有连接，返回空指针
			}
		}
	}
	/*
	shared_ptr指针调用结束析构时，会把connection指针删除，
	需要自定义删除器，将连接归还给连接池
	*/
	shared_ptr<MySQL> sp(_connectionQue.front(), [&](MySQL* pconn) {
		unique_lock<mutex> lock(_queueMutex);
		pconn->refreshAliveTime(); //刷新连接的起始空闲时间 
		_connectionQue.push(pconn);
	});
	_connectionQue.pop();
	lock.unlock();
	cv.notify_all(); // 通知生产者线程队列是否为空，以便生产新连接
	
	return sp;
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

