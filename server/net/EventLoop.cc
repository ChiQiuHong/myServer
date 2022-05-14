/**
* @description: EventLoop.cc
* @author: YQ Huang
* @brief:
* @date: 2022/05/14 09:32:41
*/

#include "server/net/EventLoop.h"

#include "server/base/Logging.h"
#include "server/base/Mutex.h"

#include "server/net/SocketsOps.h"

#include <poll.h>

#include <algorithm>
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace myserver {

namespace net {

// 线程局部变量 指向当前线程内的EventLoop对象
__thread EventLoop* t_loopInThisThread = 0;

/**
 * 构造函数
 * one loop per thread 每个线程只能有一个EventLoop对象
 * 因此构造函数会检查当前线程是否已经创建了其他EventLoop对象，遇到错误就终止程序
 * 创建了EventLoop对象的线程是IO线程 其主要功能是运行事件循环loop()
 * 对象的生命期通常和其所属的线程一样长，它不必是heap对象
 */  
EventLoop::EventLoop()
    : looping_(false),
      threadId_(CurrentThread::tid())
{
    LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
    if(t_loopInThisThread) {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread " << threadId_;
    }
    else {
        t_loopInThisThread = this;
    }
}

// 析构函数
EventLoop::~EventLoop() {
    assert(!looping_);
    t_loopInThisThread = NULL;
}

// 事件循环
void EventLoop::loop() {
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;

    ::poll(NULL, 0, 5*1000);

    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

// //
// void EventLoop::quit();

// //
// Timestamp EventLoop::pollReturnTime() const { return pollReturnTime_; }

// int64_t EventLoop::iteration() const { return iteration_; }

// //
// void EventLoop::runInLoop(Functor cb);

// //
// void EventLoop::queueInLoop(Functor cb);

// //
// size_t EventLoop::queueSize() const;

// //
// TimerId EventLoop::runAt(Timestamp time, TimerCallback cb);

// //
// TimerId EventLoop::runAfter(double delay, TimerCallback cb);

// //
// TimerId EventLoop::runEvery(double interval, TimerCallback cb);

// void EventLoop::cancel(TimerId timerId);

// void EventLoop::wakeup();
// void EventLoop::updateChannel(Channel* channel);
// void EventLoop::removeChannel(Channel* cahnnel);
// bool EventLoop::hasChannel(Channel* cahnnel);

// bool EventLoop::eventHanding() const { return eventHandling_; }

// void EventLoop::setContext(const boost::any& context) { context_ = context; }

// const boost::any& EventLoop::getContext() const { return context_; }

// boost::any* EventLoop::getMutableContext() { return &context_; }

EventLoop* EventLoop::geteventLoopOfCurrentThread() {
    return t_loopInThisThread;
}


void EventLoop::abortNotInLoopThread() {
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadid_ = " << threadId_
              << ", current thread id = " << CurrentThread::tid(); 
}


}   // namespace net

}   // namespace myserver