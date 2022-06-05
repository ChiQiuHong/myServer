/**
* @description: EventLoop.cc
* @author: YQ Huang
* @brief: 事件循环
* @date: 2022/05/14 09:32:41
*/

#include "server/net/EventLoop.h"

#include "server/base/Logging.h"
#include "server/base/Mutex.h"
#include "server/net/Channel.h"
#include "server/net/Poller.h"
#include "server/net/SocketsOps.h"
#include "server/net/TimerQueue.h"

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

// 创建事件通知描述符
int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0) {
        LOG_SYSERR << "Failed in eventfd";
        abort();
    }
    return evtfd;
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe
{
 public:
  IgnoreSigPipe()
  {
    ::signal(SIGPIPE, SIG_IGN);
    // LOG_TRACE << "Ignore SIGPIPE";
  }
};
#pragma GCC diagnostic error "-Wold-style-cast"

IgnoreSigPipe initObj;

/**
 * 构造函数
 * one loop per thread 每个线程只能有一个EventLoop对象
 * 因此构造函数会检查当前线程是否已经创建了其他EventLoop对象，遇到错误就终止程序
 * 创建了EventLoop对象的线程是IO线程 其主要功能是运行事件循环loop()
 * 对象的生命期通常和其所属的线程一样长，它不必是heap对象
 */  
EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      eventHandling_(false),
      callingPendingFunctors_(false),
      iteration_(0),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(NULL)
{
    LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
    if(t_loopInThisThread) {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread " << threadId_;
    }
    else {
        t_loopInThisThread = this;
    }
    // 设置唤醒channel的可读事件回调，并注册到Poller中
    wakeupChannel_->setReadCallback(
        std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

/**
 * 析构函数
 * 成员poller_和timerQueue_使用智能指针管理，不会手动处理资源释放的问题 
 */
EventLoop::~EventLoop() {
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
              << " destructs in thread " << CurrentThread::tid();
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}

/**
 * 定期检测是否有活跃的通道事件，有则处理，无则不处理
 * 调用Poller->poll函数，返回就绪的IO事件的channels
 * 依次执行channels中的用户回调函数
 * 接着处理IO线程中需要进行的额外计算任务
 * 
 * 当没有IO事件时poll函数最多阻塞10s，会执行一次计算任务的处理，但是
 * 10s时间太长，因此使用eventfd来唤醒poll及时处理计算任务
 */
void EventLoop::loop() {
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;  // FIXME: what if someone calls quit() before loop()
    LOG_TRACE << "EventLoop" << this << " start looping";

    while(!quit_) {
        activeChannels_.clear();
        // 调用Poller::poll()获得当前活动事件的 超时10s
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        ++iteration_;
        if(Logger::logLevel() <= Logger::TRACE) {
            printActiveChannels();
        }

        // 然后依次调用每个Channel的handleEvent函数
        eventHandling_ = true;
        for(Channel* channel : activeChannels_) {
            currentActiveChannel_ = channel;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = NULL;
        eventHandling_ = false;
        // 处理计算任务
        doPendingFunctors();
    }

    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

//
void EventLoop::quit() {
    quit_ = true;
    if(!isInLoopThread()) {
        wakeup();
    }
}

/**
 * 在IO线程内执行某个用户任务回调 两种情况
 * 1) 如果用户在当前IO线程调用这个函数，回调会同步进行
 * 2) 如果用户在其他线程调用runInLoop(), cb会被加入队列 IO线程会被唤醒来调用这个Functor
 */
void EventLoop::runInLoop(Functor cb) {
    if(isInLoopThread()) {
        cb();
    }
    else {
        // 先放入到任务队列中
        queueInLoop(std::move(cb));
    }
}

/**
 * 唤醒IO线程 两种情况
 * 1) 如果调用queueInLoop()的线程不是IO线程，那么唤醒
 * 2) 如果在IO线程调用queueInLoop()，而此时正在调用pendingFunctor 否则这些新加入的
 *    cb就不能及时调用了
 * 
 * 换句话说，只有在IO线程的事件回调中调用queueInLoop()才无须wakeup
 */
void EventLoop::queueInLoop(Functor cb) {
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }
    if(!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

// 返回待IO线程执行的任务的数量
size_t EventLoop::queueSize() const {
    MutexLockGuard lock(mutex_);
    return pendingFunctors_.size();
}

// 在某个时刻执行
TimerId EventLoop::runAt(Timestamp time, TimerCallback cb) {
    return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

// 在延迟delay之后执行
TimerId EventLoop::runAfter(double delay, TimerCallback cb) {
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, std::move(cb));
}

// 间隔interval重复执行
TimerId EventLoop::runEvery(double interval, TimerCallback cb) {
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(std::move(cb), time, interval);
}

// 取消定时器任务
void EventLoop::cancel(TimerId timerId) {
    return timerQueue_->cancel(timerId);
}

// 唤醒IO线程
void EventLoop::wakeup(){
    // 通过往eventfd写标志通知，让阻塞poll立马返回并执行回调函数
    uint64_t one = 1;
    ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one) {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << "bytes instead of 8";
    }
}

// 将事件通知描述符里的内容读走，以便让其检测事件通知
void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one) {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
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

// Channel是否注册到Poller上
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

/**
 * 执行转交给IO的计算任务
 * doPendingFunctors()没有反复执行知道pendingFunctors_为空，这是有意的
 * 否则IO线程有可能陷入死循环，无法处理IO事件
 */ 
void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    // 不是在临界区一次调用Functor，而是把回调列表swap到局部变量functors中
    // 这样减少了临界区的长度 不会阻塞其他线程调用queueuInLoop() 降低锁竞争
    // 也避免死锁的发生 因为Functor可能再调用queueInLoop()
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