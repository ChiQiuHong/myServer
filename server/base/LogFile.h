/**
* @description: LogFile.h
* @author: YQ Huang
* @brief: 日志库后端部分，负责把日志消息写到目的地
* @date: 2022/05/02 17:31:20
*/

#include "server/base/Types.h"

#include <memory>

namespace myserver {

namespace FileUtil
{
class AppendFile;
}

class LogFile {
public:
    LogFile(const string& basename, // 项目名称
            off_t rollSize, // 一次最大刷新字节数
            bool threadSafe = true, // 通过对写入操作加锁，来决定是否线程安全
            int flushInterval = 3,  // 隔多少秒刷新一次缓冲区
            int checkEveryN = 1024);
    ~LogFile();

    void append(const char* logline, int len);  // 把日志消息写到目的地
    void flush();   // 将缓冲区内的日志消息flush到硬盘
    bool rollFile();    // 日志文件滚动

private:
    void append_unlocked(const char* logline, int len); // 不加锁的append方式

    static string getLogFileName(const string& basename, time_t* now); // 获取日志文件名

    const string basename_;    // 日志 进程名称
    const off_t rollSize_;      // 日志文件达到rollSize换一个新文件
    const int flushInterval_;   // 日志写入文件间隔时间
    const int checkEveryN_;     // 每调用checkEveryN_次日志写，就滚动一次日志

    int count_; // 写入的次数

    // std::unique_ptr<MutexLock> mutex_; // 加锁
    time_t startOfPeriod_;  // 开始记录日志时间（调整到零时时间）
    time_t lastRoll_;   // 上一次滚动日志文件时间
    time_t lastFlush_;  // 上一次日志写入文件时间
    std::unique_ptr<FileUtil::AppendFile> file_; // 文件智能指针

    const static int kRollPerSeconds_ = 60 * 60 * 24;   // 一天的秒数
};

}