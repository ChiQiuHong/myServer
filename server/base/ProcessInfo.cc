/**
* @description: ProcessInfo.cc
* @author: YQ Huang
* @brief: 进程信息类
* @date: 2022/05/04 09:41:51
*/

#include "server/base/ProcessInfo.h"
#include "server/base/CurrentThread.h"
#include "server/base/FileUtil.h"

#include <algorithm>

#include <assert.h>
#include <dirent.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/times.h>

namespace myserver {

namespace detail {

// 文件操作 结构体
// struct dirent {
//     long d_ino;  // inode number 索引节点号 
//     off_t d_off; // offset to this dirent 在目录文件中的偏移
//     unsigned short d_reclen;     // 文件名长
//     unsigned char d_type;        // 文件类型
//     char d_name [NAME_MAX+1];    // 文件名 最长255字符
// }
// dirent不仅仅指向目录 还指向目录中的具体文件

// 保存某个进程打开的文件数
__thread int t_numOpenedFiles = 0;
// 获取进程打开文件数
int fdDirFilter(const struct dirent* d) {
    // 过滤条件 文件名要为数字
    if(isdigit(d->d_name[0])) {
        ++t_numOpenedFiles;
    }
    return 0;
}

// 保存某个进程的所有线程id
__thread std::vector<pid_t>* t_pids = NULL;
// 获取进程每一个线程
int taskDirFillter(const struct dirent* d) {
    // 过滤条件 文件名要为数字
    if(isdigit(d->d_name[0])) {
        t_pids->push_back(atoi(d->d_name));
    }
    return 0;
}

// 按指定过滤条件扫描dirpath
int scanDir(const char *dirpath, int (*filter)(const struct dirent *)) {
    struct dirent** namelist = NULL;
    int result = scandir(dirpath, &namelist, filter, alphasort);
    assert(namelist == NULL);
    return result;
}

Timestamp g_startTime = Timestamp::now();

// 假设它们在进程的生存期里不会发生改变
int g_clockTicks = static_cast<int>(sysconf(_SC_CLK_TCK));  // 时钟频率
int g_pageSize = static_cast<int>(sysconf(_SC_PAGE_SIZE));  // 内存页大小

}   // namespace detail

namespace ProcessInfo {

// 返回进程pid
pid_t pid() {
    return getpid();
}

// 返回进程pid的字符串格式
string pidString() {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", pid());
    return buf;
}

// 返回执行目前进程的用户识别码
uid_t uid(){
    return getuid();
}

// 返回执行目前进程的用户名
string username() {
    struct passwd pwd;
    struct passwd* result = NULL;
    char buf[8192];
    const char* name = "unknownuser";

    getpwuid_r(uid(), &pwd, buf, sizeof(buf), &result);
    if(result) {
        name = pwd.pw_name;
    }
    return name;
}

// 返回最初执行程序时所用的ID，该ID是程序的所有者
uid_t euid() {
    return geteuid();
}
// 返回进程启动时间
Timestamp startTime() {
    return detail::g_startTime;
}

// 获取时钟频率
int clockTicksPerSecond() {
    return detail::g_clockTicks;
}

// 获取内存页大小
int pageSize() {
    return detail::g_pageSize;
}

// 
bool isDebugBuild() {
#ifdef NDEBUF
    return false;
#else
    return true;
#endif
}

// 获取本地主机的标准主机名
string hostname() {
    // HOST_NAME_MAX 64
    // _POSIX_HOST_NAME_MAX 255
    char buf[256];
    if(gethostname(buf, sizeof(buf)) == 0) {
        buf[sizeof(buf) - 1] = '\0';
        return buf;
    }
    else {
        return "unknownhost";
    }
}

// 获取当前进程名称
string procname() {
    return procname(procStat()).as_string();
}

// 获取StringPiece类型的进程名称
StringPiece procname(const string& stat) {
    StringPiece name;
    size_t lp = stat.find('(');
    size_t rp = stat.rfind(')');
    if(lp != string::npos && rp != string::npos && lp < rp) {
        name.set(stat.data() + lp + 1, static_cast<int>(rp-lp-1));
    }
    return name;
}

// 获取当前进程的状态信息 相较于/proc/self/stat更加可读
string procStatus() {
    string result;
    FileUtil::readFile("/proc/self/status", 65536, &result);
    return result;
}

// 获取当前进程的状态信息
string procStat() {
    string result;
    FileUtil::readFile("/proc/self/stat", 65536, &result);
    return result;
}

// 获取当前线程的状态信息
string threadStat() {
    char buf[64];
    snprintf(buf, sizeof(buf), "/proc/self/task/%d/stat", CurrentThread::tid());
    string result;
    FileUtil::readFile(buf, 65536, &result);
    return result;
}

// 获取当前进程执行的二进制文件路径
string exePath() {
    string result;
    char buf[1024];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf));
    if(n > 0) {
        result.assign(buf, n);
    }
    return result;
}

// 获取当前进程打开的文件数
int openedFiles() {
    detail::t_numOpenedFiles = 0;
    detail::scanDir("/proc/self/fd", detail::fdDirFilter);
    return detail::t_numOpenedFiles;
}


// 获取进程能打开的最大文件描述符个数
int maxOpenFiles() {
    struct rlimit rl;
    if(getrlimit(RLIMIT_NOFILE, &rl)) {
        return openedFiles();
    }
    else {
        return static_cast<int>(rl.rlim_cur);
    }
}

// 获取进程时间
CpuTime cpuTime() {
    CpuTime t;
    struct tms tms;
    if(times(&tms) >= 0) {
        const double hz = static_cast<double>(clockTicksPerSecond());
        t.userSeconds = static_cast<double>(tms.tms_utime) / hz;
        t.systemSeconds = static_cast<double>(tms.tms_stime) / hz;
    }
    return t;
}

// 获取当前进程组的线程数
int numThreads() {
    int result = 0;
    string status = procStatus();
    size_t pos = status.find("Threads:");
    if(pos != string::npos) {
        result = atoi(status.c_str() + pos + 8);
    }
    return result;
}

// 获取当前进程的每个线程id
std::vector<pid_t> threads() {
    std::vector<pid_t> result;
    detail::t_pids = &result;
    detail::scanDir("/proc/self/task", detail::taskDirFillter);
    detail::t_pids = NULL;
    std::sort(result.begin(), result.end());
    return result;
}

}   // namespace ProcessInfo

}   // namespace myserver