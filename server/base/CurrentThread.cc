/**
* @description: CurrentThread.cc
* @author: YQ Huang
* @brief: 实现对线程id的管理和优化
* @date: 2022/05/03 14:12:22
*/

#include "server/base/CurrentThread.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>

namespace myserver {

namespace CurrentThread {

__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char* t_threadName = "unknown";
static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

string stackTrace(bool demangle) {
    string stack;
    return stack;
}

}   // namespace CurrentThread

}   // namespace myserver