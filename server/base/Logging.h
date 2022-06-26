/**
* @description: Logging.h
* @author: YQ Huang
* @brief: 日志库前端部分，供应用程序使用的接口，并生成日志消息
* @date: 2022/05/01 08:42:44
*/

#pragma once

#include "server/base/LogStream.h"
#include "server/base/Timestamp.h"

namespace myserver {

/**
 * Logger类，作用是生成日志消息
 * 使用Impl设计模式，真正的实现类是内部类Impl
 * 默认消息格式为：日期-时间-微秒-线程-级别-正文-源文件名-行号
 */
class Logger {
public:
    enum LogLevel {
        TRACE,  // 指出比DEBUG粒度更细的一些信息事件（开发过程中使用）
        DEBUG,  // 指出细粒度事件对调式应用程序是非常有帮助的。（开发过程中使用）
        INFO,   // 表明消息在粗粒度级别上突出强调应用程序的运行过程
        WARN,   // 系统能正常运行，但可能会出现潜在错误的情形。
        ERROR,  // 指出虽然发生错误事件，但仍然不影响系统的继续运行。
        FATAL,  // 指出每个严重的错误事件将会导致应用程序的退出。
        NUM_LOG_LEVELS,
    };

    /**
     * SourceFile用于在输出信息的时候指明是在具体的出错文件
     * 如(/muduo/base/Logging.cc变为Logging.cc)
     */
    class SourceFile {
    public:
        template<int N>
        SourceFile(const char (&arr)[N])
            : data_(arr),
              size_(N-1)
        {
            // strrchr 返回str中最后一次出现字符的位置
            const char* slash = strrchr(data_, '/');
            if(slash) {
                data_ = slash + 1;
                size_ -= static_cast<int>(data_ - arr);
            }
        }

        explicit SourceFile(const char* filename)
            : data_(filename)
        {
            const char* slash = strrchr(filename, '/');
            if(slash) {
                data_ = slash + 1;
            }
            size_ = static_cast<int>(strlen(data_));
        }

        const char* data_;  // 文件名
        int size_;  // 文件名的长度
    };

    // 构造函数 调用内部类impl来实现 根据参数将信息加到输出缓冲区
    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line,  LogLevel level);
    Logger(SourceFile file, int line,  LogLevel level, const char* func);
    Logger(SourceFile file, int line, bool toAbort);
    // 析构函数 把LogStream缓冲区中的内容取出来，由g_output控制输出到特定文件
    ~Logger();

    // 返回LogStream的引用 LogStream重载了<<运算符，因此可以直接使用
    LogStream& stream() { return impl_.stream_; }

    // static 静态的 访问它不需要实例对象就能访问
    static LogLevel logLevel();
    static void setLogLevel(LogLevel level);

    typedef void (*OutputFunc)(const char* msg, int len);   // 函数指针
    typedef void (*FlushFunc)();        // 函数指针
    static void setOutput(OutputFunc);  // 默认 fwrite 到 stdout
    static void setFlush(FlushFunc);    // 默认 fflush 到 stdout

private:
    /**
     * Impl是真正实现的类
     * 将日志事件（时间 日志级别 文件名 行号等）信息加到输出缓冲区
     */
    class Impl {
    public:
        typedef Logger::LogLevel LogLevel;
        Impl(LogLevel level, int old_errno, const SourceFile& file, int line);    // fix file
        void formatTime();  // 设置时间
        void finish();  // 结束

        Timestamp time_;    // 时间 fix
        LogStream stream_;  // LogStream对象 fix
        LogLevel level_;    // 当前日志级别
        int line_;          // 行号 由__line__得到
        SourceFile basename_;   // 文件名 由__file__与sourcefile类得到
    };

    Impl impl_; // 实现对象

};

// 全局变量
extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel() {
    return g_logLevel;
}

// 设置一些方便使用的宏
#define LOG_TRACE if(myserver::Logger::logLevel() <= myserver::Logger::TRACE) \
    myserver::Logger(__FILE__, __LINE__, myserver::Logger::TRACE, __func__).stream()
#define LOG_DEBUG if(myserver::Logger::logLevel() <= myserver::Logger::DEBUG) \
    myserver::Logger(__FILE__, __LINE__, myserver::Logger::DEBUG, __func__).stream()
#define LOG_INFO if(myserver::Logger::logLevel() <= myserver::Logger::INFO) \
    myserver::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN myserver::Logger(__FILE__, __LINE__, myserver::Logger::WARN).stream()
#define LOG_ERROR myserver::Logger(__FILE__, __LINE__, myserver::Logger::ERROR).stream()
#define LOG_FATAL myserver::Logger(__FILE__, __LINE__, myserver::Logger::FATAL).stream()
#define LOG_SYSERR myserver::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL myserver::Logger(__FILE__, __LINE__, true).stream()

// 根据错误码得到对应的错误描述
const char* strerror_tl(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.
#define CHECK_NOTNULL(val) \
    ::myserver::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// A small helper for CHECK_NOTNULL().
template<typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char *names, T* ptr) {
    if(ptr == NULL) {
        Logger(file, line, Logger::FATAL).stream() << names;
    }
    return ptr;
}

}   // namespace myserver