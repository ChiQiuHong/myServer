/**
* @description: Condition.h
* @author: YQ Huang
* @brief: 条件变量类 跟mutexlock配合使用
* @date: 2022/05/03 18:49:35
*/

#pragma once

#include "server/base/Mutex.h"

#include <pthread.h>

namespace myserver {

class Condition : noncopyable {
public:
    // 构造函数 初始化条件变量
    explicit Condition(MutexLock& mutex)
        : mutex_(mutex)
    {
        MCHECK(pthread_cond_init(&pcond_, NULL));
    }

    // 析构函数 销毁条件变量
    ~Condition() {
        MCHECK(pthread_cond_destroy(&pcond_));
    }
    
    // 如果等待条件没有被满足，则调用p_c_w一直等待下去，直到被唤醒
    // pthread_cond_wait会自动解锁mutex，条件变量满足后会重新加锁
    void wait() {
        MutexLock::UnassignGuard ug(mutex_);
        pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
    }

    // 内部实现为非阻塞版本的p_c_tw 超过设定的时间后立即返回
    bool waitForSeconds(double seconds);

    // p_c_s一次唤醒一个线程，如果有多个线程都调用wait，那随机唤醒
    void notify() {
        MCHECK(pthread_cond_signal(&pcond_));
    }

    // 唤醒阻塞所有调用wait的线程
    void notifyAll() {
        MCHECK(pthread_cond_broadcast(&pcond_));
    }

private:
    MutexLock& mutex_;      // 互斥体对象
    pthread_cond_t pcond_;  // 条件变量
};

}   // namespace myservesr