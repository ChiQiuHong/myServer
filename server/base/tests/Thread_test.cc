/**
* @description: Thread_test.cc
* @author: YQ Huang
* @brief: Thread类测试函数
* @date: 2022/05/03 11:46:51
*/

#include "server/base/Thread.h"
#include "server/base/Logging.h"

#include <string>
#include <stdio.h>
#include <unistd.h>

void threadFunc() {
    printf("Hello This is Thread\n");
}

int main() {
    
    myserver::Thread t1(threadFunc, "t1");
    t1.start();
    sleep(1);
    printf("t1.tid=%d\n", t1.tid());
    LOG_INFO << "Test Success";
    t1.join();
}