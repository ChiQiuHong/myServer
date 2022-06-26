/**
* @description: EventLoopThread.cc
* @author: YQ Huang
* @brief: EventLoopThread类封装了IO线程
* @date: 2022/06/26 17:01:25
*/

#include "server/net/EventLoopThread.h"

#include "server/net/EventLoop.h"


namespace myserver {

namespace net {

/**
 * @brief 封装了一个EventLoop，提供接口开始执行事件循环、获取
 *        创建的EventLoop对象
 */

// 构造 创建一个线程用于执行事件循环
EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                    const string& name)
    : loop_(NULL),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(mutex_),
      callback_(cb)
{
}

// 析构，退出事件循环，关闭线程
EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if(loop_ != NULL) {
        loop_->quit();
        thread_.join();
    }
}

// 启动线程，执行线程函数，阻塞等到线程函数通知，最后返回创建的EventLoop对象指针
EventLoop* EventLoopThread::startLoop() {
    assert(!thread_.started());
    thread_.start();    // 启动线程 执行线程函数

    EventLoop* loop = NULL;
    {
        MutexLockGuard lock(mutex_);
        while(loop_ == NULL) {
            cond_.wait();   // 保证线程函数准备工作完成，EventLoop指针对象不为空
        }
        loop = loop_;
    }
    return loop;
}

// 线程函数
void EventLoopThread::threadFunc() {
    EventLoop loop;
    if(callback_) {
        callback_(&loop);   // 执行初始化回调函数
    }

    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify(); // 初始化成功，通知用户启动成功
    }

    loop.loop();    // 线程中执行事件循环
    MutexLockGuard lock(mutex_);
    loop_ = NULL;
}

}   // namespace net

}   // namespace myserver
