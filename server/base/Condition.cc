/**
* @description: Condition.cc
* @author: YQ Huang
* @brief: 条件变量类 跟mutexlock配合使用
* @date: 2022/05/03 18:49:47
*/

#include "server/base/Condition.h"

#include <errno.h>

namespace myserver {

// 超时返回true 其他情况返回false
bool Condition::waitForSeconds(double seconds) {
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);

    const int64_t kNanoSecondsPerSecond = 1000000000;
    int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);

    abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
    abstime.tv_nsec += static_cast<long>((abstime.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);

    MutexLock::UnassignGuard ug(mutex_);
    return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime);
}

}   // namespace myserver