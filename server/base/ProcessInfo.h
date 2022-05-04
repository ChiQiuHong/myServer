/**
* @description: ProcessInfo.h
* @author: YQ Huang
* @brief: 进程信息类
* @date: 2022/05/04 09:43:56
*/

#pragma once

#include "server/base/StringPiece.h"
#include "server/base/Types.h"
#include "server/base/Timestamp.h"

#include <vector>
#include <sys/types.h>

namespace myserver {

namespace ProcessInfo {

/**
 * 下面定义了一系列关于读取进程信息的函数
 * /proc文件系统 包含了各种用于展示内核信息的文件 并允许进程通过常规I/O系统读取
 * 获取于进程有关的信息 /proc/PID 每个进程可以通过/proc/self来访问自己的目录
 */

pid_t pid();                // 返回进程pid
string pidString();         // 返回进程pid的字符串格式
uid_t uid();                // 返回执行目前进程的用户识别码
string username();          // 返回执行目前进程的用户名
uid_t euid();               // 返回最初执行程序时所用的ID，该ID是程序的所有者
Timestamp startTime();      // 返回进程启动时间
int clockTicksPerSecond();  // 获取时钟频率
int pageSize();             // 获取内存页大小
bool isDebugBuild();

string hostname();          // 获取本地主机的标准主机名
string procname();          // 获取当前进程名称
StringPiece procname(const string& stat);   // 获取StringPiece类型的进程名称

string procStatus();        // 获取当前进程的状态信息 相较于/proc/self/stat更加可读
string procStat();          // 获取当前进程的状态信息
string threadStat();        // 获取当前线程的状态信息
string exePath();           // 获取当前进程执行的二进制文件路径

int openedFiles();          // 获取当前进程打开的文件数
int maxOpenFiles();         // 获取进程能打开的最大文件描述符个数

struct CpuTime {
    double userSeconds;
    double systemSeconds;

    CpuTime() : userSeconds(0.0), systemSeconds(0.0) { }

    double total() const { return userSeconds + systemSeconds; }
};

CpuTime cpuTime();           // 获取进程时间

int numThreads();            // 获取当前进程组的线程数
std::vector<pid_t> threads();// 获取当前进程的每个线程id

}   // namespace ProcessInfo

}   // namespace myserver