/**
* @description: ThreadPool_test.cc
* @author: YQ Huang
* @brief: 线程池 测试函数
* @date: 2022/05/05 18:37:32
*/

#include "server/base/ThreadPool.h"
#include "server/base/CountDownLatch.h"
#include "server/base/CurrentThread.h"
#include "server/base/Logging.h"

#include <stdio.h>
#include <unistd.h>

void print() {
    printf("tid=%d\n", myserver::CurrentThread::tid());
}

void printString(const std::string& str) {
    LOG_INFO << str;
    usleep(100*1000);
}

void test(int maxSize) {
    LOG_WARN << "Test ThreadPool with max queue size = " << maxSize;
    myserver::ThreadPool pool("MainThreadPool");
    pool.setMaxQueueSize(maxSize);
    pool.start(5);

    LOG_WARN << "Adding";
    pool.run(print);
    pool.run(print);
    for(int i = 0; i < 100; ++i) {
        char buf[32];
        snprintf(buf, sizeof(buf), "task %d", i);
        pool.run(std::bind(printString, std::string(buf)));
    }
    LOG_WARN << "Done";

    // 下面这段的意思是
    // 由于任务总在队列队头中取，所以latch是最后才执行的 latch没有执行前 主线程一直是阻塞的
    // 这样就保证了任务队列为空了，也就是任务都做完了，主线程才关闭线程池
    myserver::CountDownLatch latch(1);
    pool.run(std::bind(&myserver::CountDownLatch::countDown, &latch));
    latch.wait(); 
    pool.stop();
}

void longTask(int num) {
    LOG_INFO << "longTask " << num;
    myserver::CurrentThread::sleepUsec(3000000);
}

void test2() {
    LOG_WARN << "Test ThreadPool by stoping early";
    myserver::ThreadPool pool("ThreadPool");
    pool.setMaxQueueSize(5);
    pool.start(3);

    myserver::Thread thread1([&pool]()
    {
        for(int i = 0; i < 20; ++i) {
            pool.run(std::bind(longTask, i));
        }
    }, "thread1");
    thread1.start();

    myserver::CurrentThread::sleepUsec(5000000);
    LOG_WARN << "stop pool";
    pool.stop();    // early stop

    thread1.join();
    pool.run(print);
    LOG_WARN << "test2 Done";
}

int main() {
    test(0);
    test(1);
    test(5);
    test(10);
    test(50);
    test2();
}