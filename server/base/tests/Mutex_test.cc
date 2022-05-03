/**
* @description: Mutex_test.cc
* @author: YQ Huang
* @brief: Mutex测试函数
* @date: 2022/05/03 21:21:47
*/

#include "server/base/CountDownLatch.h"
#include "server/base/Mutex.h"
#include "server/base/Thread.h"
#include "server/base/Timestamp.h"

#include <vector>
#include <stdio.h>

myserver::MutexLock g_mutex;
std::vector<int> g_vec;
const int kCount = 10*1000*1000;

void threadFunc() {
    for(int i = 0; i < kCount; ++i) {
        myserver::MutexLockGuard lock(g_mutex);
        g_vec.push_back(i);
    }
}

int foo() __attribute__ ((noinline));

int g_count = 0;
int foo() {
    myserver::MutexLockGuard lock(g_mutex);
    if(!g_mutex.isLockedByThisThread()) {
        printf("FAIL\n");
        return -1;
    }
    ++g_count;
    return 0;
}

int main() {
    printf("sizeof pthread_mutex_t: %zd\n", sizeof(pthread_mutex_t));
    printf("sizeof Mutex: %zd\n", sizeof(myserver::MutexLock));
    printf("sizeof pthread_cond_t: %zd\n", sizeof(pthread_cond_t));
    printf("sizeof Condition: %zd\n", sizeof(myserver::Condition));
    MCHECK(foo());
    if(g_count != 1) {
        printf("MCHECK calls twice. \n");
        abort();
    }

    const int kMaxThreads = 8;
    g_vec.reserve(kMaxThreads * kCount);

    myserver::Timestamp start(myserver::Timestamp::now());
    for(int i = 0; i < kCount; ++i) {
        g_vec.push_back(i);
    }

    printf("single thread without lock %f\n", myserver::timeDifference(myserver::Timestamp::now(), start));
    
    g_vec.clear();
    start = myserver::Timestamp::now();
    threadFunc();
    printf("single thread with lock %f\n", myserver::timeDifference(myserver::Timestamp::now(), start));

    for(int nthreads = 1; nthreads < kMaxThreads; ++nthreads) {
        std::vector<std::unique_ptr<myserver::Thread> > threads;
        g_vec.clear();
        start = myserver::Timestamp::now();
        for(int i = 0; i < nthreads; ++i) {
            threads.emplace_back(new myserver::Thread(&threadFunc));
            threads.back()->start();
        }
        for(int i = 0; i < nthreads; ++i) {
            threads[i]->join();
        }
        printf("%d thread(s) with lock %f\n", nthreads, myserver::timeDifference(myserver::Timestamp::now(), start));
    }

}