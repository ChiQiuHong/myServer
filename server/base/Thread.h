/**
* @description: Thread.h
* @author: YQ Huang
* @brief: 对pthread封装
* @date: 2022/05/03 10:07:18
*/

#pragma once

#include "server/base/Atomic.h"
#include "server/base/CountDownLatch.h"
#include "server/base/Types.h"

#include <functional>
#include <memory>
#include <pthread.h>

namespace myserver {

/**
 * 对pthread的高级封装
 * 为什么不用c++11的thread类 要自己封装？性能原因
 * 主要使用的是线程的真实id tid 只能通过linux的系统调用syscall(SYS_gettid)获得
 * 不能每次想要知道tid的时候都调用一次，很浪费时间，而且对于主线程来说，也没有办法直接
 * 获得某个子线程的tid 在这里 用一个ThreadData中间结构体缓存信息
 */ 
class Thread : noncopyable {
public:
    typedef std::function<void ()> ThreadFunc;

    // 构造函数
    explicit Thread(ThreadFunc, const string& name = string());
    // 析构函数
    ~Thread();
    
    // 线程执行函数
    void start();   
    // 等待线程执行完成
    int join();     

    // 判断线程是否开始运行
    bool started() const { return started_; }
    // 返回线程tid
    pid_t tid() const { return tid_; }
    // 返回线程名称
    const string& name() const { return name_; }
    // 返回当前已经创建的线程数量
    static int numCreated() { return numCreated_.get(); }

private:
    void setDefalutName();  // 默认线程名称

    bool started_;          // 线程是否开始运行
    bool joined_;           // 线程是否可以join
    pthread_t pthreadId_;   // 线程变量
    pid_t tid_;             // 线程tid
    ThreadFunc func_;       // 线程执行函数
    string name_;           // 线程名称
    CountDownLatch latch_;  // 线程同步

    static AtomicInt32 numCreated_; // 原子操作 当前已经创建线程的数量

};

}   // namespace myserver

