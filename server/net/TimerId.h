/**
* @description: TimerId.h
* @author: YQ Huang
* @brief: 记录定时器Id的抽象类
* @date: 2022/05/21 19:00:12
*/

#pragma once

#include "server/base/copyable.h"

namespace myserver {

namespace net {

class Timer;

/**
 * TimerId 内部类
 * 封装Timer指针和其序列号，设计用来取代Timer的
 * 使用pair<Timer*, int64_t> 来表示一个唯一的定时器
 * 因为Timer*无法表示区分前后两次创建但可能地址相同的情况，增加一个id来确保唯一性
 */
class TimerId : public myserver::copyable {
public:
    TimerId()
        : timer_(NULL),
          sequence_(0)
    { }

    TimerId(Timer* timer, int64_t seq)
        : timer_(timer),
          sequence_(seq)
    { }

    // 声明了一个友元类，方便TimerQueue能直接访问TimerId的私有成员变量
    friend class TimerQueue;
    
private:
    Timer* timer_;      // 指向定时器的指针
    int64_t sequence_;  // 定时器的序号
};

}   // namespace net

}   // namespace myserver