/**
* @description: ProcessInfo_test.cc
* @author: YQ Huang
* @brief: 进程信息类 测试函数
* @date: 2022/05/04 11:09:47
*/

#include "server/base/ProcessInfo.h"
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

int main() {
    printf("pid = %d\n", myserver::ProcessInfo::pid());
    printf("uid = %d\n", myserver::ProcessInfo::uid());
    printf("euid = %d\n", myserver::ProcessInfo::euid());
    printf("start time = %s\n", myserver::ProcessInfo::startTime().toFormattedString().c_str());
    printf("hostname = %s\n", myserver::ProcessInfo::hostname().c_str());
    printf("opened files = %d\n", myserver::ProcessInfo::openedFiles());
    printf("threads = %zd\n", myserver::ProcessInfo::threads().size());
    printf("num threads = %d\n", myserver::ProcessInfo::numThreads());
    printf("status = %s\n", myserver::ProcessInfo::procStatus().c_str());
}