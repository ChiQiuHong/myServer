/**
* @description: Logging.h
* @author: YQ Huang
* @brief: 日志库前端部分，供应用程序使用的接口，并生成日志消息
* @date: 2022/05/01 08:42:44
*/

#pragma once

#include <string>
#include <sstream>

// 设置一些方便使用的宏
#define LOG_TRACE if(myserver::Logger::logLevel() <= myserver::Logger::TRACE) \
    myserver::Logger(__FILE__, __LINE__).stream().str()

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

    // 构造函数
    Logger(const char* file, int line);

    ~Logger();

    std::ostream& stream() { return impl_.stream_; }

    static LogLevel logLevel();
    static void setLogLevel(LogLevel level);    //设置日志级别

private:
    class Impl {
    public:
        typedef Logger::LogLevel LogLevel;
        Impl(LogLevel level, int old_errno, const char* file, int line);    // fix file
        void formatTime();  // 设置时间
        void finish();  // 结束

        uint64_t time_ = 0; // 时间 fix
        std::ostream stream_;  // LogStream对象 fix
        LogLevel level_;    // 当前日志级别
        int line_;          // 行号 由__line__得到
        const char* basename_ = nullptr;   // 文件名 由__file__与sourcefile类得到
    };

    Impl impl_; // 实现对象

};

// 全局变量
extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel() {
    return g_logLevel;
}

}