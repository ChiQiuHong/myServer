/**
* @description: CountDownLatch.cc
* @author: YQ Huang
* @brief: 倒计时
* @date: 2022/05/03 20:22:05
*/

#include "server/base/CountDownLatch.h"

namespace myserver {

// 构造函数
CountDownLatch::CountDownLatch(int count)
    : mutex_(),
      condition_(mutex_),
      count_(count) 
{
}

// 计数大于0 阻塞等待
void CountDownLatch::wait() {
    MutexLockGuard lock(mutex_);
    while(count_ > 0) {
        condition_.wait();
    }
}

// 计数-1 等于0的话唤醒线程
void CountDownLatch::countDown() {
    MutexLockGuard lock(mutex_);
    --count_;
    if(count_ == 0) {
        condition_.notifyAll();
    }
}

// 获取计数
int CountDownLatch::getCount() const {
    MutexLockGuard lock(mutex_);
    return count_;
}

}   // namespace myserver