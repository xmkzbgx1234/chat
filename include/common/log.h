#ifndef LOG_HPP
#define LOG_HPP

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

enum class LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance; // C++11 线程安全的局部静态变量
        return instance;
    }

    void setLogLevel(LogLevel level) { minLevel_ = level; }

    // 设置输出到文件
    void setOutputFile(const std::string& filename) {
        fileStream_.open(filename, std::ios::app);
        if (fileStream_.is_open()) {
            outStream_ = &fileStream_;
        }
    }

    // 核心写入方法（被 LogStream 调用）
    void write(LogLevel level, const char* file, int line, const std::string& message) {
        if (level < minLevel_) return;

        std::lock_guard<std::mutex> lock(mutex_); // 自动加锁解锁

        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);

        *outStream_ << "[" << formatTime(time_t_now) << "] "
            << "[" << levelToString(level) << "] "
            << baseName(file) << ":" << line << " - "
            << message << std::endl;

        // 如果是 ERROR 或者强制刷新，可以立即 flush
        if (level == LogLevel::ERROR) {
            outStream_->flush();
        }
    }

    LogLevel getMinLevel() const { return minLevel_; }

private:
    Logger() : minLevel_(LogLevel::INFO), outStream_(&std::cout) {}
    ~Logger() { if (fileStream_.is_open()) fileStream_.close(); }

    // 禁止拷贝
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    const char* baseName(const char* path) const {
        const char* file = path;
        for (const char* p = path; *p != '\0'; ++p) {
            if (*p == '/' || *p == '\\') {
                file = p + 1;
            }
        }
        return file;
    }

    std::string formatTime(std::time_t t) {
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        return std::string(buffer);
    }

    const char* levelToString(LogLevel level) {
        switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
        }
    }

    LogLevel minLevel_;
    std::mutex mutex_;
    std::ofstream fileStream_;
    std::ostream* outStream_;
};

class LogStream {
public:
    LogStream(LogLevel level, const char* file, int line)
        : level_(level), file_(file), line_(line) {}

    ~LogStream() {
        Logger::getInstance().write(level_, file_, line_, stream_.str());
    }

    template<typename T>
    LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

private:
    LogLevel level_;
    const char* file_;
    int line_;
    std::ostringstream stream_;
};

// 宏：增加级别判断，实现短路优化
#define LOG_STREAM(level) \
    if ((level) < Logger::getInstance().getMinLevel()) {} \
    else LogStream((level), __FILE__, __LINE__)

#define LOG_DEBUG LOG_STREAM(LogLevel::DEBUG)
#define LOG_INFO LOG_STREAM(LogLevel::INFO)
#define LOG_WARN LOG_STREAM(LogLevel::WARN)
#define LOG_ERROR LOG_STREAM(LogLevel::ERROR)

#endif //
