/**
* @description: Timer.cc
* @author: YQ Huang
* @brief: 定时器类
* @date: 2022/05/21 19:24:17
*/

#include "server/net/Timer.h"

namespace myserver {

namespace net {

AtomicInt64 Timer::s_numCreated_;

// 重启定时器
void Timer::restart(Timestamp now) {
    // 如果是重复执行的，到期的时间就为当前时间加上定时时间
    if(repeat_) {
        expiration_ = addTime(now, interval_);
    }
    // 如果不是，到期时间为非法值 等于停止使用这个定时器
    else {
        expiration_ = Timestamp::invalid();
    }
}


}   // namespace net

}   // namespace myserver