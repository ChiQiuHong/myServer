/**
* @description: LogFile.cc
* @author: YQ Huang
* @brief: 日志库后端部分，负责把日志消息写到目的地
* @date: 2022/05/02 17:31:24
*/

#include "server/base/LogFile.h"
#include "server/base/FileUtil.h"
#include "server/base/ProcessInfo.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>

namespace myserver {

// LogFile 构造函数
LogFile::LogFile(const string& basename,
                 off_t rollSize,
                 bool threadSafe,
                 int flushInterval,
                 int checkEveryN)
    : basename_(basename),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      checkEveryN_(checkEveryN),
      count_(0),
      mutex_(threadSafe ? new MutexLock : NULL),
      startOfPeriod_(0),
      lastRoll_(0),
      lastFlush_(0)
{
    // 检查basename是否是最小路径单位
    assert(basename.find('/') == string::npos);
    // 滚动初始化
    rollFile();
}

// 默认析构函数
LogFile::~LogFile() = default;

// 把日志消息写到缓冲区 具体实现是私有函数append_unlocked
void LogFile::append(const char* logline, int len) {
    if(mutex_) {
        MutexLockGuard lock(*mutex_);
        append_unlocked(logline, len);
    }
    else {
        append_unlocked(logline, len);
    }
    
}

/**
 * 将缓冲区内的日志消息flush到硬盘
 */
void LogFile::flush() {
    if(mutex_) {
        MutexLockGuard lock(*mutex_);
        file_->flush();
    }
    else {
        file_->flush();
    }
}

/**
 * 把日志消息写到缓冲区
 * 1) 定期（默认3秒）将缓冲区内的日志消息flush到硬盘
 * 2）24小时滚动一次，不管文件写没写满
 * 3）文件写满了触发滚动
 */
void LogFile::append_unlocked(const char* logline, int len) {
    // 写入缓冲区
    file_->append(logline, len);

    // 触发滚动条件1：文件写满了
    if(file_->writtenBytes() > rollSize_) {
        rollFile();
    }
    else {
        // 对于每秒写几十万条日志的高性能日志库，如果每次写入都查看是否满足下面条件
        // 就太浪费性能了，所以设置count参数，每1024次才检查一次。
        ++count_;
        if(count_ >= checkEveryN_) {
            count_ = 0;
            time_t now = time(NULL);
            time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
            // 触发滚动条件2：24小时滚动一次
            if(thisPeriod_ != startOfPeriod_) {
                rollFile();
            }
            // 判断是否超过flush间隔时间，超过了就flush，否则什么都不做
            else if(now - lastFlush_ > flushInterval_) { 
                lastFlush_ = now;
                file_->flush();
            }
        }
    }
}

/**
 * 日志文件滚动 条件
 * 1) 达到文件大小，例如每写满1GB就换下一个文件
 * 2) 时间，例如每天零点新建一个日志文件，不论前一个文件有没有写满
 */
bool LogFile::rollFile() {
    time_t now = 0;
    string filename = getLogFileName(basename_, &now);
    // 将start对齐到kR的整数倍，也就是时间调整到当天零时
    // now是1970.1.1零时到现在的秒数
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    // 若当前天数大于lastRoll的天数
    if (now > lastRoll_) {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start; //记录上一次rollfile的日期（天）
        // 换一个文件写日志，即为了保证两天的日志不写在同一个文件中，而上一天的日志可能并未写到rollSize_大小 
        file_.reset(new FileUtil::AppendFile(filename));
        return true;
    }
    return false;
}

/** 典型的日志文件的文件名如下：
 *  logfile_test.20210603-144022.hostname.3602.log 
 * 进程名字 + 文件的创建时间 + 机器名称 + 进程id + 后缀名.log
 */
string LogFile::getLogFileName(const string& basename, time_t* now) {
    
    string filename;
    filename.reserve(basename.size() + 64); // 为容器预留足够的空间
    filename = basename;

    char timebuf[32];
    struct tm t_time;
    *now = time(NULL);
    localtime_r(now, &t_time);
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &t_time);
    filename += timebuf;

    filename += ProcessInfo::hostname();

    char pidbuf[32];
    snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
    filename += pidbuf;

    filename += ".log";

    return filename;
}   

}   // namespace myserver 