/**
* @description: Logging.cc
* @author: YQ Huang
* @brief: 日志库前端部分，供应用程序使用的接口，并生成日志消息
* @date: 2022/05/01 08:42:51
*/

#include "server/base/Logging.h"

#include "server/base/Timestamp.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

namespace myserver {

// 缓存错误信息
char t_errnobuf[512];
/**
 * 如果前后两次日志操作，都是在一秒钟内，那仅格式化微秒部分
 * 缓存时间和秒数
 */
char t_time[64];        // 时间字符串 "Y:M:D H:M:S"
time_t t_lastSecond;    // 当前线程上一次日志记录时的秒数

const char* strerror_tl(int savedErrno) {
    // 根据错误码得到对应的错误描述
    return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

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

// 帮助类 在编译时就知道字符串的长度
class T {
public:
    T(const char* str, unsigned len)
        : str_(str),
          len_(len)
    {
        assert(strlen(str) == len_);
    }

    const char* str_;
    const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v) {
    s.append(v.str_, v.len_);
    return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v) {
    s.append(v.data_, v.size_);
    return s;
}

// 默认输出函数 写出到stdout
void defaultOutput(const char* msg, int len) {
    size_t n = fwrite(msg, 1, len, stdout);
    (void)n;
}

// 默认flush函数 flush到stdout
void defaultFlush() {
    fflush(stdout);
}

// 全局变量
Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

// Impl构造函数
Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
    : time_(Timestamp::now()),
      stream_(),
      level_(level),
      line_(line),
      basename_(file)
{
    formatTime();
    stream_ << T(LogLevelName[level], 6);
    if(savedErrno != 0) {
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

// 设置时间
void Logger::Impl::formatTime() {
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();    // 获得微秒
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);   // 获得秒
    int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);    // 微妙
    // 如果日志记录不在同一秒，则更新日志记录时间
    if(seconds != t_lastSecond) {
        t_lastSecond= seconds;
        struct tm tm_time;
        localtime_r(&seconds, &tm_time);

        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
            tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17);
        (void)len;
    }
    // 更新微秒
    Fmt us(".%6dZ ", microseconds);
    assert(us.length() == 9);
    stream_ << T(t_time, 17) << T(us.data(), 9);
}

// 结束
void Logger::Impl::finish() {
    stream_ << " - " << basename_ << ':' << line_ << '\n';
}

// 构造函数 调用内部类impl来实现 根据参数将信息加到输出缓冲区
Logger::Logger(SourceFile file, int line)
    : impl_(INFO, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level)
    : impl_(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
    : impl_(level, 0, file, line)
{
    impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, bool toAbort)
    : impl_(toAbort ? FATAL:ERROR, errno, file, line)
{
}

// 析构函数 把LogStream缓冲区中的内容取出来，由g_output控制输出到特定文件
Logger::~Logger() {
    impl_.finish();
    const LogStream::Buffer& buf(stream().buffer());
    // 调用g_output函数，将存于LogStream的buffer的日志内容输出
    g_output(buf.data(), buf.length());
    // 如果是FATAL错误，还要调用g_flush立即从缓冲区中输出，然后终止程序
    if(impl_.level_ == FATAL) {
        g_flush();
        abort();
    }
}

//设置日志级别
void Logger::setLogLevel(Logger::LogLevel level) {
    g_logLevel = level;
}

// 设置输出函数
void Logger::setOutput(OutputFunc out) {
    g_output = out;
}

// 设置Flush函数
void Logger::setFlush(FlushFunc flush) {
    g_flush = flush;
}

} // namespace myserver