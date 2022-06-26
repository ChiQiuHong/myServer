/**
* @description: EventLoopThread.h
* @author: YQ Huang
* @brief: EventLoopThread类封装了IO线程
* @date: 2022/06/26 17:02:36
*/

#pragma once

#include "server/base/Condition.h"
#include "server/base/Mutex.h"
#include "server/base/Thread.h"

namespace myserver {

namespace net {

class EventLoop;

class EventLoopThread : noncopyable {
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                    const string& name = string());
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop* loop_;   // 指向当前线程创建的EventLoop对象
    bool exiting_;      // 退出线程标志位
    Thread thread_;     // 线程
    MutexLock mutex_;   // 互斥锁
    Condition cond_;    // 用于保证初始化成功
    ThreadInitCallback callback_;   // 线程初始化的回调函数
};

}   // namespace net

}   // namespace myserver