/**
* @description: TimerQueue.cc
* @author: YQ Huang
* @brief: 定时器队列
* @date: 2022/05/21 10:41:19
*/

#include "server/net/TimerQueue.h"

#include "server/base/Logging.h"
#include "server/net/EventLoop.h"
#include "server/net/Timer.h"
#include "server/net/TimerId.h"

#include <sys/timerfd.h>
#include <unistd.h>

namespace myserver {

namespace net {

namespace detail {

/**
 * 调用timer_create()创建特定的定时器
 * CLOCK_MONOTONIC 以固定的速率运行，系统启动后不会被改变
 * TFD_NONBLOCK 设置文件描述符非阻塞
 * TFD_CLOEXEC  设置文件描述符close-on-exec，即使用exec执行程序时，关闭该文件描述符
 */
int createTimerfd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    if(timerfd < 0) {
        LOG_SYSFATAL << "Failed in timerfd_create";
    }
    return timerfd;
}

/**
 * 时间结构体
 * struct timespec {
 *      time_t tv_sec;      // 秒
 *      long   tv_nsec;     // 纳秒
 * };
 */
struct timespec howMuchTimeFromNow(Timestamp when) {
    int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();

    if(microseconds < 100) {
        microseconds = 100;
    }

    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(
        microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(
        (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

/**
 * 读取文件描述符
 * 当定时器到期后，需要对timerfd进行read操作以告知内核在fd上的事件已接受并处理
 * 否则可能被Poller重新监听
 */
void readTimerfd(int timerfd, Timestamp now) {
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if(n != sizeof howmany) {
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}

/**
 * 设置timerfd的定时器
 * new_value参数指定定时器的初始过期时间和间隔
 * old_value参数返回定时器这次设置之前的到期时间
 * 
 * struct itimerspec {
 *      struct timespec it_interval;    // 触发到期的间隔时间
 *      struct timespec it_value;       // 初始到期时间
 * };
 */
void resetTimerfd(int timerfd, Timestamp expiration) {
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memZero(&newValue, sizeof newValue);
    memZero(&oldValue, sizeof oldValue);
    // it_value表示启动定时器后，定时器第一次定时到期的时间
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if(ret) {
        LOG_SYSERR << "timerfd_settime()";
    }
}

}   // namespace detail

/**
 * 构造函数 通过调用detail::createTimerfd() 创建定时器的文件描述符
 * 并设置回调函数
 */
TimerQueue::TimerQueue(EventLoop* loop) 
    : loop_(loop),
      timerfd_(detail::createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_(),
      callingExpiredTimers_(false)
{
    // 设置Channel的可读事件的回调 设置关注可读事件
    // 并交由EventLoop管理，加入Poller监听队列
    timerfdChannel_.setReadCallback(
        std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

/**
 * 析构函数 关闭文件描述符
 * 删除所有定时器
 */
TimerQueue::~TimerQueue() {
    // 取消注册timefd事件 从EventLoop、Poller移除监听
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    
    for(const Entry& timer : timers_) {
        delete timer.second;
    }
}

/**
 * addTimer()是供EventLoop使用并被封装成更好用的runAt()、runAfter()等
 * 线程安全的，可以跨线程调用
 * 创建一个Timer并添加到定时器队列中，并返回一个TimerId
 */
TimerId TimerQueue::addTimer(TimerCallback cb,
                             Timestamp when,
                             double interval)
{
    Timer* timer = new Timer(std::move(cb), when, interval);
    // 添加到定时器队列
    // 调用了EventLoop::runInLoop() 作为计算任务放入队列中在loop函数中执行，保证线程安全
    loop_->runInLoop(
        std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

/**
 * 取消一个定时器
 */
void TimerQueue::cancel(TimerId timerId) {
    loop_->runInLoop(
        std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

/**
 *  在当前IO循环中新增一个定时器
 */
void TimerQueue::addTimerInLoop(Timer* timer) {
    loop_->assertInLoopThread();
    // 插入一个新的定时器
    // 有可能会使得最早到期的定时器发生改变
    bool earliestChanged = insert(timer);
    
    // 需要更新
    if(earliestChanged) {
        // 重置，重新设定timerfd的到期时间
        detail::resetTimerfd(timerfd_, timer->expiration());
    }
}

/**
 * 在当前IO循环中取消一个定时器
 */
void TimerQueue::cancelInLoop(TimerId timerId) {
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    // 通过TimerId从未超时的队列中activeTimers_查找timer
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if(it != activeTimers_.end()) {
        // 从定时器队列中删除Timer 并且释放Timer对象
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1);
        (void)n;
        delete it->first;
        activeTimers_.erase(it);
    }
    // 特殊情况
    else if(callingExpiredTimers_) {
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

/**
 * 定时器到期处理
 */
void TimerQueue::handleRead() {
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    // 对timefd进行读操作，避免重复触发到期事件
    detail::readTimerfd(timerfd_, now);

    // 获取当前时刻到期的定时器列表
    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;   // 开始处理到期的定时器
    cancelingTimers_.clear();       // 清理要取消的定时器

    for(const Entry& it : expired) {
        it.second->run();   // 执行回调函数
    }
    callingExpiredTimers_ = false;

    reset(expired, now);    // 处理到期的定时器，重新激活还是删除
}

/**
 *  从timers_中移除已到期的Timer，并通过vector返回它们
 */ 
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now){
    assert(timers_.size() == activeTimers_.size());
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    // lower_bound() 二分查找 返回第一个大于等于x的数
    TimerList::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);
    // 拷贝到期的迭代器
    std::copy(timers_.begin(), end, back_inserter(expired));
    // 从定时器队列中删除到期的定时器
    timers_.erase(timers_.begin(), end);

    // 从activeTimers_中移除到期的定时器
    for(const Entry& it : expired) {
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1);
        (void) n;
    }

    assert(timers_.size() == activeTimers_.size());
    return expired;
}

/**
 *  重置定时器 两种情况
 *  1) 需要重复执行，重新设定超时事件，并作为新的定时器添加到activeTimers_中
 *  2) 不需要重复执行，销毁Timer对象即可
 */
void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now) {
    Timestamp nextExpire;
    for(const Entry& it : expired) {
        ActiveTimer timer(it.second, it.second->sequence());
        // 需要重复执行并且不是代取消的定时器
        if(it.second->repeat()
            && cancelingTimers_.find(timer) == cancelingTimers_.end())
        {
            it.second->restart(now);    // 重启定时器
            insert(it.second);
        }
        else {
            delete it.second;
        }
    }

    // 重置定时器有可能会使定时器队列里最早到期的定时器发生改变
    // 需要更新timerfd
    if(!timers_.empty()) {
        nextExpire = timers_.begin()->second->expiration();
    }
    if(nextExpire.valid()) {
        detail::resetTimerfd(timerfd_, nextExpire);
    }
}

/**
 * 把定时器插入到timers_和activeTimers_队列中 
 * 返回最早到期的时间是否发生改变
 */
bool TimerQueue::insert(Timer* timer) {
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;           // 最早到期的时间是否发生改变
    Timestamp when = timer->expiration();   // 新加入的定时器的到期时间
    TimerList::iterator it = timers_.begin();
    // 新加入的定时器的到期时间要小于现有最先到期的定时器的到期时间
    if(it == timers_.end() || when < it->first) {
        earliestChanged = true;
    }
    // 添加到timers_中
    {
        std::pair<TimerList::iterator, bool> result
            = timers_.insert(Entry(when, timer));
        assert(result.second);
        (void)result;
    }
    // 添加到activeTimers_中
    {
        std::pair<ActiveTimerSet::iterator, bool> result
            = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second);
        (void)result;
    }

    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}


}   // namespace net

}   // namespace myserver