/**
* @description: Mutex.h
* @author: YQ Huang
* @brief: 互斥器类
* @date: 2022/05/03 17:17:34
*/

#pragma once

#include "server/base/CurrentThread.h"
#include "server/base/noncopyable.h"

#include <assert.h>
#include <pthread.h>

namespace myserver {

/**
 * 用RAII手法封装mutex的创建、销毁、加锁、解锁这四个操作
 */
class MutexLock : noncopyable {
public:
    // 构造函数 创建mutex
    MutexLock() {
        pthread_mutex_init(&mutex_, NULL);
    }

    // 析构函数 销毁mutex
    ~MutexLock() {
        pthread_mutex_destroy(&mutex);
    }

    // 加锁
    void lock() {
        pthread_mutex_lock(&mutex_);
    }

    // 解锁
    void unlock() {
        pthread_mutex_unlock(&mutex_);
    }

    // 获取mutex对象
    pthread_mutex_t* getPthreadMutex() {
        return &mutex_;
    }

private:
    pthread_mutex_t mutex_; 
}

/**
 * MutexLock管理类
 * 用RAII手法实现构造时给互斥加锁 析构时解锁
 */
class MutexLockGuard : noncopyable {
public:
    // 构造函数 给互斥加锁
    explicit MutexLockGuard(MutexLock& mutex)
        : mutex_(mutex)
    {
        mutex.lock();
    }

    ~MutexLockGuard() {
        mutex_.unlock();
    }

private:
    MutexLock& mutex_;
}

// 防止程序里出现临时对象MutexLockGuard 要加变量名！
#define MutexLockGuard(x) error "Missing guard object name"

}   // namespace myserver