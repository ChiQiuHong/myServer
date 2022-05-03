/**
* @description: Thread_test.cc
* @author: YQ Huang
* @brief: Thread类测试函数
* @date: 2022/05/03 11:46:51
*/

#include "server/base/Thread.h"
#include "server/base/CurrentThread.h"
#include "server/base/Logging.h"

#include <string>
#include <stdio.h>
#include <unistd.h>

void threadFunc() {
    printf("tid=%d\n", myserver::CurrentThread::tid());
}

void threadFunc2(int x) {
    printf("tid=%d, x=%d\n", myserver::CurrentThread::tid(), x);
}

int main() {

    printf("pid=%d, tid=%d\n", getpid(), myserver::CurrentThread::tid());
    
    myserver::Thread t1(threadFunc);
    t1.start();
    printf("t1.tid=%d\n", t1.tid());
    LOG_INFO << "Test Success";
    t1.join();

    myserver::Thread t2(std::bind(threadFunc2, 42), "thread for free function with argument");
    t2.start();
    printf("t2.tid=%d\n", t2.tid());
    LOG_INFO << "Test Success";
    t2.join();
}