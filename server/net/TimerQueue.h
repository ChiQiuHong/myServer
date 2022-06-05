/**
* @description: TimerQueue.h
* @author: YQ Huang
* @brief: 定时器队列
* @date: 2022/05/21 10:41:24
*/

#pragma once

#include <set>
#include <vector>

#include "server/base/Mutex.h"
#include "server/base/Timestamp.h"
#include "server/net/Callbacks.h"
#include "server/net/Channel.h"

namespace myserver {

namespace net {

class EventLoop;
class Timer;
class TimerId;

/**
 * TimerQueue维护了一系列定时器，当定时器到期后会产生一个定时器事件，Poller就会返回activeChannels
 * 首先回调的是handleEvent() handleEvent()又根据activeChannels是什么事件，调用TimeQueue的handleRead()
 * TimeQueue的handleRead()又会获取所有的超时的定时器，然后回调用户的回调函数
 * 
 * 传统的Reactor通过控制select和poll的等待时间来实现定时
 * 而现在在linux中有了timerfd，我们可以用和处理IO事件相同的方式来处理定时
 */
class TimerQueue : noncopyable {
public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerId addTimer(TimerCallback cb,
                     Timestamp when,
                     double interval);

    void cancel(TimerId timerId);

private:
    /**
     * TimerQueue需要高效地组织目前尚未到期的Timer，能快速地根据当前时间找到已经到期的Timer
     * 也要能高效地添加和删除Timer。
     * 使用二叉搜索树（std::set/std::map) 把Timer按到期时间先后排好序
     * 操作的复杂度是O(logN)
     * 
     * TimerQueue实际使用了std::set，其key为std::pair<Timestamp, Timer*>
     * 不直接使用map<Timestamp, Timer*>的原因是
     * 无法处理两个Timer到期时间相同的情况
     * 区分key后，即使两个Timer的到期时间相同，它们的地址也必定不同。
     */
    // FIXME 这里不应该使用Timer指针对象，可以使用std::unique_ptr管理
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;
    typedef std::pair<Timer*, int64_t> ActiveTimer;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    void addTimerInLoop(Timer* timer);
    void cancelInLoop(TimerId timerId);
    
    void handleRead();

    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    bool insert(Timer* timer);

    EventLoop* loop_;           // TimeQueue所属的EventLoop
    const int timerfd_;         // 定时器文件描述符
    Channel timerfdChannel_;    // 定时器文件描述符所对应的事件分发器
    TimerList timers_;          // 按到期时间先后排序好的定时器队列

    // for cancel()
    ActiveTimerSet activeTimers_;       // 还没到期的定时器队列
    bool callingExpiredTimers_;         // 是否正在处理到期的定时器
    ActiveTimerSet cancelingTimers_;    // 代取消的定时器队列
};

}   // namespace net

}   // namespace myserver