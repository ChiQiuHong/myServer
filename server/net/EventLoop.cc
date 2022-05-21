/**
* @description: EventLoop.cc
* @author: YQ Huang
* @brief:
* @date: 2022/05/14 09:32:41
*/

#include "server/net/EventLoop.h"

#include "server/base/Logging.h"
#include "server/base/Mutex.h"
#include "server/net/Channel.h"
#include "server/net/Poller.h"
#include "server/net/SocketsOps.h"

#include <algorithm>
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace myserver {

namespace net {

// 线程局部变量 指向当前线程内的EventLoop对象
__thread EventLoop* t_loopInThisThread = 0;
// 传递给epoll_wait timeout的参数 这是等待10秒
const int kPollTimeMs = 10000;

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

/**
 * 定期检测是否有活跃的通道事件，有则处理，无则不处理
 * 
 */
void EventLoop::loop() {
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG_TRACE << "EventLoop" << this << " start looping";

    while(!quit_) {
        activeChannels_.clear();
        // 调用Poller::poll()获得当前活动事件的
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        ++iteration_;
        if(Logger::logLevel() <= Logger::TRACE) {
            printActiveChannels();
        }

        // 然后依次调用每个Channel的handleEvent函数
        eventHandling_ = true;
        for(Channel* channel : activeChannels_) {
            currentActiveChannel_ = channel;
            currentActiveChannel_->handleEvent();
        }
        currentActiveChannel_ = NULL;
        eventHandling_ = false;
        // doPendingFunctors();
    }

    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

//
void EventLoop::quit() {
    quit_ = true;
}

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

// 唤醒事件通知描述符
void EventLoop::wakeup(){
    uint64_t one = 1;
    ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one) {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << "bytes instead of 8";
    }
}

/**
 * updateChannel()在检查断言之后调用Poller::updateChannel()
 * EventLoop不关心Poller是如何管理Channel列表的
 */
void EventLoop::updateChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

// 删除事件分发器 调用Poller::removeChannel()
void EventLoop::removeChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if(eventHandling_) {
        assert(currentActiveChannel_ == channel ||
            std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}

// 判断 调用Poller::hasChannel()
bool EventLoop::hasChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return poller_->hasChannel(channel);
}

// 返回当前线程内的EventLoop对象
EventLoop* EventLoop::geteventLoopOfCurrentThread() {
    return t_loopInThisThread;
}

// 不在IO线程里
void EventLoop::abortNotInLoopThread() {
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadid_ = " << threadId_
              << ", current thread id = " << CurrentThread::tid(); 
}

// 执行转交给IO的任务
void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    // 这里使用了缩减临界区代码的技巧，减少锁的争用
    // 相当于剪切操作
    // 这里之所以会出现并发的问题，是因为本函数不会跨线程
    // 但是runInLoop可以跨函数，会更改pendingFunctors_
    // 所以这里需要加锁
    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor& functor : functors) {
        functor();
    }
    callingPendingFunctors_ = false;
}

// DEBUG 将发生的事件写入日志
void EventLoop::printActiveChannels() const {
    for(const Channel* channel : activeChannels_) {  
        LOG_TRACE << "{" << channel->reventsToString() << "} ";
    }
}

}   // namespace net

}   // namespace myserver