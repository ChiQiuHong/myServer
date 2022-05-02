/**
* @description: Timestamp.h
* @author: YQ Huang
* @brief: 时间戳
* @date: 2022/05/02 09:58:04
*/

#pragma once

#include "server/base/copyable.h"

#include <string>
#include <boost/operators.hpp>

namespace myserver {

/**
 * Timestamp类主要用于日志、定时器模块中记录时间戳、打印时间戳字符串
 */
class Timestamp : public myserver::copyable,
                  public boost::equality_comparable<Timestamp>,
                  public boost::less_than_comparable<Timestamp>
{
public:
    // 默认的时间戳的微秒记录为0
    Timestamp()
        : microSecondsSinceEpoch_(0)
    { }

    explicit Timestamp(int64_t microSecondsSinceEpochArg)
        : microSecondsSinceEpoch_(microSecondsSinceEpochArg)
    { }

    void swap(Timestamp& that) {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }

    // 格式化时间戳的字符串
    std::string toString() const;
    std::string toFormattedString(bool showMicroseconds = true) const;

    // 判断时间戳是否有效
    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    // 获取时间戳 微秒 和 秒
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    time_t secondsSinceEpoch() const {
        return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    }

    // 当前时间戳
    static Timestamp now();
    static Timestamp invalid() { return Timestamp(); }

    // 从time_t 微秒 偏移 得到时间戳
    static Timestamp fromUnixTime(time_t t) { return fromUnixTime(t, 0); }
    static Timestamp fromUnixTime(time_t t, int microseconds) {
        return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
    }

    static const int kMicroSecondsPerSecond = 1000 * 1000;  // 一秒 = 1000000微秒

private:
    int64_t microSecondsSinceEpoch_;    // 记录了自1970.1.1到现在的微秒数
};

// 时间戳比较 小于
inline bool operator<(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

// 时间戳比较 等于
inline bool operator==(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

// 计算两个时间戳的时间间隔
inline double timeDifference(Timestamp high, Timestamp low) {
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

// 时间戳加上偏移
inline Timestamp addTime(Timestamp timestamp, double seconds) {
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

}   // namespace myserver