/**
* @description: EventLoop.h
* @author: YQ Huang
* @brief:
* @date: 2022/05/14 09:32:26
*/

#include <atomic>
#include <functional>
#include <vector>

#include <boost/any.hpp>

#include "server/base/Mutex.h"
#include "server/base/CurrentThread.h"
#include "server/base/Timestamp.h"
// #include "server/net/Callbacks.h"
// #include "server/net/TimerId.h"

namespace myserver {

namespace net {

class EventLoop : noncopyable {
public:
    typedef std::function<void()> Functor;
    // 构造函数
    EventLoop();
    // 析构函数
    ~EventLoop();

    // 
    void loop();

    // //
    // void quit();

    // //
    // Timestamp pollReturnTime() const { return pollReturnTime_; }
    
    // int64_t iteration() const { return iteration_; }

    // //
    // void runInLoop(Functor cb);

    // //
    // void queueInLoop(Functor cb);

    // //
    // size_t queueSize() const;

    // //
    // TimerId runAt(Timestamp time, TimerCallback cb);

    // //
    // TimerId runAfter(double delay, TimerCallback cb);

    // //
    // TimerId runEvery(double interval, TimerCallback cb);

    // void cancel(TimerId timerId);

    // void wakeup();
    // void updateChannel(Channel* channel);
    // void removeChannel(Channel* cahnnel);
    // bool hasChannel(Channel* cahnnel);

    //
    void assertInLoopThread() {
        if(!isInLoopThread()) {
            abortNotInLoopThread();
        }
    }

    //
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    // bool eventHanding() const { return eventHandling_; }

    // void setContext(const boost::any& context) { context_ = context; }
    
    // const boost::any& getContext() const { return context_; }

    // boost::any* getMutableContext() { return &context_; }

    static EventLoop* geteventLoopOfCurrentThread();

private:
    void abortNotInLoopThread();
    // void handleRead();
    // void doPendingFunctors();

    // void printActiveChannels() const;

    // typedef std::vector<Channel*> ChannelList;

    bool looping_;
    // std::atomic<bool> quit_;
    // bool eventHandling_;
    // bool callingPendingFunctors_;
    // int64_t iteration_;
    const pid_t threadId_;
    // Timestamp pollReturnTime_;
    // std::unique_ptr<Poller> poller_;
    // std::unique_ptr<TimerQueue> tiemrQueue_;
    // int wakeupFd_;
    // std::unique_ptr<Channel> wakeupChannel_;
    // boost::any context_;

    // ChannelList activeChannels_;
    // Channel* currentActiveChannel_;

    // mutable MutexLock mutex_;
    // std::vector<Functor> pendingFunctors_ GUARDED_BY(mutex_);
};

}   // namespace net

}   // namespace myserver