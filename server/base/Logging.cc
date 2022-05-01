/**
* @description: Logging.cc
* @author: YQ Huang
* @brief: 日志库前端部分，供应用程序使用的接口，并生成日志消息
* @date: 2022/05/01 08:42:51
*/

#include "server/base/Logging.h"

#include <iostream>
#include <time.h>

namespace myserver {

// 帮助把level int类型转换为字符串类型
const char* LogLevelName[Logger::NUM_LOG_LEVELS] = 
{
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};

// 对g_logLevel进行初始化
Logger::LogLevel initLogLevel() {
    if(getenv("SERVER_LOG_TRACE"))
        return Logger::TRACE;
    else if(getenv("SERVER_LOG_DEBUG"))
        return Logger::DEBUG;
    else
        return Logger::INFO;
}

Logger::LogLevel g_logLevel = initLogLevel();


// Impl构造函数
Logger::Impl::Impl(LogLevel level, int old_errno, const char* file, int line)
    : time_(0),
      stream_(),
      level_(level),
      line_(line),
      basename_(file)
{
    formatTime();
    stream_ << LogLevelName[level];
}

// 设置时间
void Logger::Impl::formatTime() {
    struct tm tm;
    time_t curTime = time(NULL);
    localtime_r(&curTime, &tm);
    char buf[64];
    strftime(buf, sizeof buf, "%Y%m%d %H:%M:%S ", &tm);
    stream_ << buf;
}

// 结束
void Logger::Impl::finish() {
    stream_ << " - " << basename_ << ':' << line_ << '\n';
    std::cout << "error" << std::endl;
}

// 构造函数 调用内部类impl来实现
Logger::Logger(const char* file, int line)
    : impl_(INFO, 0, file, line)
{
}

// 析构函数
Logger::~Logger() {
    impl_.finish();
}

//设置日志级别
void Logger::setLogLevel(Logger::LogLevel level) {
    g_logLevel = level;
}

} // namespace myserver