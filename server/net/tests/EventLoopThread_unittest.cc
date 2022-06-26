/**
* @description: EventLoopThread_unittest.cc
* @author: YQ Huang
* @brief: EventLoopThread类测试函数
* @date: 2022/06/26 18:00:31
*/

#include "server/net/EventLoopThread.h"
#include "server/net/EventLoop.h"
#include "server/base/Thread.h"
#include "server/base/CountDownLatch.h"

#include <stdio.h>
#include <unistd.h>

using namespace myserver;
using namespace myserver::net;

void print(EventLoop* p = NULL) {
    printf("print: pid = %d, tid = %d, loop = %p\n",
           getpid(), CurrentThread::tid(), p);
}

void quit(EventLoop* p) {
    print(p);
    p->quit();
}

int main() {
    print();

    {
        EventLoopThread thr1; // never start
    }

    {
        EventLoopThread thr2;
        EventLoop* loop = thr2.startLoop();
        loop->runInLoop(std::bind(print, loop));
        CurrentThread::sleepUsec(500 * 1000);
    }

    {
        EventLoopThread thr3;
        EventLoop* loop = thr3.startLoop();
        loop->runInLoop(std::bind(quit, loop));
        CurrentThread::sleepUsec(500 * 1000);
    }
}