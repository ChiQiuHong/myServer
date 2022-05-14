/**
* @description: EventLoop_unittest.cc
* @author: YQ Huang
* @brief: EventLoop 单元测试
* @date: 2022/05/14 12:42:17
*/

#include "server/net/EventLoop.h"
#include "server/base/Thread.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

using namespace myserver;
using namespace myserver::net;

EventLoop* g_loop;

void threadFunc() {
    printf("threadFunc(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    EventLoop loop;
    loop.loop();
}

int main() {
    printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    EventLoop loop;
    Thread thread(threadFunc);
    thread.start();

    loop.loop();
    pthread_exit(NULL);
}