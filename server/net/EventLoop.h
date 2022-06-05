/**
* @description: EventLoop.h
* @author: YQ Huang
* @brief: 事件循环
* @date: 2022/05/14 09:32:26
*/

#pragma once

#include <atomic>
#include <functional>
#include <vector>

#include <boost/any.hpp>

#include "server/base/Mutex.h"
#include "server/base/CurrentThread.h"
#include "server/base/Timestamp.h"
#include "server/net/Callbacks.h"
#include "server/net/TimerId.h"

namespace myserver {

namespace net {

class Channel;
class Poller;
class TimerQueue;

/**
 * one loop per thread 每个线程最多只能有一个EventLoop对象
 * 创建了EventLoop对象的线程称为IO线程，其功能是运行事件循环
 */
class EventLoop : noncopyable {
public:
    typedef std::function<void()> Functor;
    // 构造函数
    EventLoop();
    // 析构函数
    ~EventLoop();

    // 事件循环
    void loop();

    // 退出事件循环
    void quit();

    // 返回poll return的时刻
    Timestamp pollReturnTime() const { return pollReturnTime_; }
    // 事件循环的次数
    int64_t iteration() const { return iteration_; }

    // 在IO线程内运行某个用户任务回调
    void runInLoop(Functor cb);

    // 
    void queueInLoop(Functor cb);

    //
    size_t queueSize() const;

    
    TimerId runAt(Timestamp time, TimerCallback cb);     // 在某个时刻执行
    TimerId runAfter(double delay, TimerCallback cb);    // 在延迟delay之后执行
    TimerId runEvery(double interval, TimerCallback cb); // 间隔interval重复执行
    void cancel(TimerId timerId);                        // 取消定时器任务

    void wakeup();                          // 唤醒IO线程
    void updateChannel(Channel* channel);   // 更新事件分发器
    void removeChannel(Channel* channel);   // 删除事件分发器
    bool hasChannel(Channel* channel);      // Channel是否注册到Poller上

    // 判断是否在当前线程运行
    void assertInLoopThread() {
        if(!isInLoopThread()) {
            abortNotInLoopThread();
        }
    }

    // 判断是否在当前线程运行
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    // 判断EventLoop是否在分发事件
    bool eventHanding() const { return eventHandling_; }

    void setContext(const boost::any& context) { context_ = context; }
    
    //
    const boost::any& getContext() const { return context_; }

    //
    boost::any* getMutableContext() { return &context_; }

    // 返回当前线程内的EventLoop对象
    static EventLoop* geteventLoopOfCurrentThread();

private:
    void abortNotInLoopThread();    // 不在IO线程里
    void handleRead();              // 将事件通知描述符里的内容读走，以便让其检测事件通知
    void doPendingFunctors();       // 执行转交给IO的任务

    void printActiveChannels() const;   // DEBUG 将发生的事件写入日志

    typedef std::vector<Channel*> ChannelList;  // 事件分发器列表

    bool looping_;                      // 是否运行循环
    std::atomic<bool> quit_;            // 是否退出事件循环
    bool eventHandling_;                // EventLoop是否在分发事件
    bool callingPendingFunctors_;       // EventLoop是否在处理任务
    int64_t iteration_;                 // 事件循环的次数
    const pid_t threadId_;              // 运行loop的线程ID
    Timestamp pollReturnTime_;          // poll阻塞的时间
    std::unique_ptr<Poller> poller_;    // IO复用
    std::unique_ptr<TimerQueue> timerQueue_; // 定时器列表
    int wakeupFd_;                           // eventfd描述符，用于唤醒阻塞的Poller
    std::unique_ptr<Channel> wakeupChannel_; // eventfd对应的Channel
    boost::any context_;                     //

    ChannelList activeChannels_;        // 活跃的事件列表
    Channel* currentActiveChannel_;     // 当前处理的事件

    mutable MutexLock mutex_;           // 互斥锁
    std::vector<Functor> pendingFunctors_;  // 需要在主IO线程执行的任务
};

}   // namespace net

}   // namespace myserver