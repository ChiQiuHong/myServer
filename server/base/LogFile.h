/**
* @description: LogFile.h
* @author: YQ Huang
* @brief: 日志库后端部分，负责把日志消息写到目的地
* @date: 2022/05/02 17:31:20
*/

#pragma once

#include "server/base/Types.h"
#include "server/base/noncopyable.h"

#include <memory>

namespace myserver {

namespace FileUtil
{
class AppendFile;
}

class LogFile : noncopyable{
public:
    LogFile(const string& basename, 
            off_t rollSize,         
            bool threadSafe = true,
            int flushInterval = 3,
            int checkEveryN = 1024);
    ~LogFile();

    void append(const char* logline, int len);  // 把日志消息写到缓冲区
    void flush();   // 将缓冲区内的日志消息flush到硬盘
    bool rollFile();    // 日志文件滚动

private:
    void append_unlocked(const char* logline, int len); // 不加锁的append方式

    static string getLogFileName(const string& basename, time_t* now); // 获取日志文件名

    const string basename_;     // 日志文件名，默认保存在当前工作目录下
    const off_t rollSize_;      // 日志文件超过设定值进行roll
    const int flushInterval_;   // flush刷新时间间隔
    const int checkEveryN_;     // 每1024次日志操作，检查是否刷新，是否roll

    int count_; // 记录写入的次数

    // std::unique_ptr<MutexLock> mutex_; // 加锁
    time_t startOfPeriod_;  // 开始记录日志时间（调整到零时时间）
    time_t lastRoll_;       // 上一次滚动日志文件时间
    time_t lastFlush_;      // 上一次日志写入文件时间
    std::unique_ptr<FileUtil::AppendFile> file_; // 文件智能指针

    const static int kRollPerSeconds_ = 60 * 60 * 24;   // 一天的秒数
};

}   // namespace myserver