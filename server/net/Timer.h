/**
* @description: Timer.h
* @author: YQ Huang
* @brief: 定时器类
* @date: 2022/05/21 19:14:54
*/

#pragma once

#include "server/base/Atomic.h"
#include "server/base/Timestamp.h"
#include "server/net/Callbacks.h"

namespace myserver {

namespace net {

/**
 * 定时器 内部类 
 * 封装了定时器的一些参数，例如超时回调函数、超时时间、定时器是否重复、重复时间间隔、定时器的序列号
 */
class Timer : noncopyable {
public:
    // 定时器构造函数
    Timer(TimerCallback cb, Timestamp when, double interval)
        : callback_(std::move(cb)),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          sequence_(s_numCreated_.incrementAndGet())
    { }

    // 到期执行回调函数
    void run() const {
        callback_();
    }

    // 返回定时器的到期时间
    Timestamp expiration() const { return expiration_; }
    // 返回定时器是否重复执行
    bool repeat() const { return repeat_; }
    // 返回定时器序号
    int64_t sequence() const { return sequence_; }

    // 重启定时器
    void restart(Timestamp now);

    // 返回当前定时器数目
    static int64_t numCreated() { return s_numCreated_.get(); }
    
private:
    const TimerCallback callback_;  // 定时器回调函数
    Timestamp expiration_;          // 定时器到期的时间    
    const double interval_;         // 定时器重复执行的时间间隔，如不重复，设为非正值 
    const bool repeat_;             // 定时器是否需要重复执行
    const int64_t sequence_;        // 定时器的序号

    static AtomicInt64 s_numCreated_;   // 定时器的计数值
};

}   // namespace net

}   // namespace myserver