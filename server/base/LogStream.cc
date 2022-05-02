/**
* @description: LogStream.cc
* @author: YQ Huang
* @brief: LogStream类
* @date: 2022/05/01 16:01:25
*/

#include "server/base/LogStream.h"

#include <algorithm>
#include <type_traits>
#include <stdio.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

// beter itoa
#pragma GCC diagnostic ignored "-Wtype-limits"

namespace myserver {

namespace detail {

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;  // 指向了digits数组中的0
static_assert(sizeof(digits) == 20, "wrong number of digits");

const char digitsHex[] = "0123456789ABCDEF";
static_assert(sizeof digitsHex == 17, "wrong number of digitsHex");

/**
 * 将Value转化成字符串形式，by Matthew Wilson.
 * 将转换结果保存到buf，返回字符串的长度
 */
template<typename T>
size_t convert(char buf[], T value) {
    T i = value;
    char* p = buf;

    // 循环 从低位到高位逐位读取数字 注意 读出来的是逆序的
    do {
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = zero[lsd];
    } while(i != 0);

    // 如果value为负数，要加负号
    if(value < 0) {
        *p++ = '-';
    }

    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}

/**
 * 将Value转换成16进制
 * uintprt_t对于32位平台来说就是unsigned int
 * 对于64位平台来说就是unsigned long
 */
size_t convertHex(char buf[], uintptr_t value) {
    uintptr_t i = value;
    char* p = buf;

    do {
        int lsd = static_cast<int>(i % 16);
        i /= 16;
        *p++ = digitsHex[lsd];
    } while(i != 0);

    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}

// FixedBuffer实例化
template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

}   // namespace detail

template<int SIZE>
const char* detail::FixedBuffer<SIZE>::debugString() {
    *cur_ = '\0';
    return data_;
}

template<int SIZE>
void detail::FixedBuffer<SIZE>::cookieStart()
{
}

template<int SIZE>
void detail::FixedBuffer<SIZE>::cookieEnd()
{
}

/**
 * 静态检查 用于检查一些类型的大小
 * std::numeric_limits<T>::digits10 表示类型T能无更改地表示的最大10位数
 */
void LogStream::staticCheck() {
    // double 15
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10,
                  "kMaxNumericSize is large enough");
    // long double 18
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10,
                  "kMaxNumericSize is large enough");
    // long 18
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10,
                  "kMaxNumericSize is large enough");
    // long long 18
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10,
                  "kMaxNumericSize is large enough");
}

/**
 * 模板类 将T类型转为10进制
 * 内部调用convert函数，并将结果保存到buffer中
 */
template<typename T>
void LogStream::formatInteger(T v) {
    if(buffer_.avail() >= kMaxNumericSize) {
        size_t len = detail::convert(buffer_.current(), v);
        buffer_.add(len);
    }
}

/**
 * 整型数据的字符串转换、保存到缓冲区；内部均调用formatInteger<T>函数
 */
LogStream& LogStream::operator<<(short v) {
    *this << static_cast<int>(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned short v) {
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream& LogStream::operator<<(int v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned int v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long long v) {
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long long v) {
    formatInteger(v);
    return *this;
}

/**
 * 指针类型转换为16进制字符串
 */
LogStream& LogStream::operator<<(const void* p) {
    uintptr_t v = reinterpret_cast<uintptr_t>(p);
    if(buffer_.avail() >= kMaxNumericSize) {
        char* buf = buffer_.current();
        buf[0] = '0';
        buf[1] = 'x';
        size_t len = detail::convertHex(buf+2, v);
        buffer_.add(len+2);
    }
    return *this;
}

/**
 * 浮点类型数据转换为字符串；内部使用snprintf函数
 * 有时间可以看看Grisu3算法 by Florian Loitsch
 */
LogStream& LogStream::operator<<(double v) {
    if(buffer_.avail() >= kMaxNumericSize) {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v); // 以十进制输出小数点后12位
        buffer_.add(len);
    }
    return *this;
}

template<typename T>
Fmt::Fmt(const char* fmt, T val) {
    // 断言T是算术类型
    static_assert(std::is_arithmetic<T>::value == true, "Must be arithmetic type");
    length_ = snprintf(buf_, sizeof(buf_), fmt, val);
    assert(static_cast<size_t>(length_) < sizeof(buf_));
}

// 实例化
template Fmt::Fmt(const char* fmt, char);

template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);

template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);

/*
 Format a number with 5 characters, including SI units.
 [0,     999]
 [1.00k, 999k]
 [1.00M, 999M]
 [1.00G, 999G]
 [1.00T, 999T]
 [1.00P, 999P]
 [1.00E, inf)
*/
string formatSI(int64_t s) {
    double n = static_cast<double>(s);
    char buf[64];
    if (s < 1000)
        snprintf(buf, sizeof(buf), "%" PRId64, s);
    else if (s < 9995)
        snprintf(buf, sizeof(buf), "%.2fk", n/1e3);
    else if (s < 99950)
        snprintf(buf, sizeof(buf), "%.1fk", n/1e3);
    else if (s < 999500)
        snprintf(buf, sizeof(buf), "%.0fk", n/1e3);
    else if (s < 9995000)
        snprintf(buf, sizeof(buf), "%.2fM", n/1e6);
    else if (s < 99950000)
        snprintf(buf, sizeof(buf), "%.1fM", n/1e6);
    else if (s < 999500000)
        snprintf(buf, sizeof(buf), "%.0fM", n/1e6);
    else if (s < 9995000000)
        snprintf(buf, sizeof(buf), "%.2fG", n/1e9);
    else if (s < 99950000000)
        snprintf(buf, sizeof(buf), "%.1fG", n/1e9);
    else if (s < 999500000000)
        snprintf(buf, sizeof(buf), "%.0fG", n/1e9);
    else if (s < 9995000000000)
        snprintf(buf, sizeof(buf), "%.2fT", n/1e12);
    else if (s < 99950000000000)
        snprintf(buf, sizeof(buf), "%.1fT", n/1e12);
    else if (s < 999500000000000)
        snprintf(buf, sizeof(buf), "%.0fT", n/1e12);
    else if (s < 9995000000000000)
        snprintf(buf, sizeof(buf), "%.2fP", n/1e15);
    else if (s < 99950000000000000)
        snprintf(buf, sizeof(buf), "%.1fP", n/1e15);
    else if (s < 999500000000000000)
        snprintf(buf, sizeof(buf), "%.0fP", n/1e15);
    else
        snprintf(buf, sizeof(buf), "%.2fE", n/1e18);
    return buf;
}

/*
 [0, 1023]
 [1.00Ki, 9.99Ki]
 [10.0Ki, 99.9Ki]
 [ 100Ki, 1023Ki]
 [1.00Mi, 9.99Mi]
*/
string formatIEC(int64_t s) {
    double n = static_cast<double>(s);
    char buf[64];
    const double Ki = 1024.0;
    const double Mi = Ki * 1024.0;
    const double Gi = Mi * 1024.0;
    const double Ti = Gi * 1024.0;
    const double Pi = Ti * 1024.0;
    const double Ei = Pi * 1024.0;

    if (n < Ki)
        snprintf(buf, sizeof buf, "%" PRId64, s);
    else if (n < Ki*9.995)
        snprintf(buf, sizeof buf, "%.2fKi", n / Ki);
    else if (n < Ki*99.95)
        snprintf(buf, sizeof buf, "%.1fKi", n / Ki);
    else if (n < Ki*1023.5)
        snprintf(buf, sizeof buf, "%.0fKi", n / Ki);

    else if (n < Mi*9.995)
        snprintf(buf, sizeof buf, "%.2fMi", n / Mi);
    else if (n < Mi*99.95)
        snprintf(buf, sizeof buf, "%.1fMi", n / Mi);
    else if (n < Mi*1023.5)
        snprintf(buf, sizeof buf, "%.0fMi", n / Mi);

    else if (n < Gi*9.995)
        snprintf(buf, sizeof buf, "%.2fGi", n / Gi);
    else if (n < Gi*99.95)
        snprintf(buf, sizeof buf, "%.1fGi", n / Gi);
    else if (n < Gi*1023.5)
        snprintf(buf, sizeof buf, "%.0fGi", n / Gi);

    else if (n < Ti*9.995)
        snprintf(buf, sizeof buf, "%.2fTi", n / Ti);
    else if (n < Ti*99.95)
        snprintf(buf, sizeof buf, "%.1fTi", n / Ti);
    else if (n < Ti*1023.5)
        snprintf(buf, sizeof buf, "%.0fTi", n / Ti);

    else if (n < Pi*9.995)
        snprintf(buf, sizeof buf, "%.2fPi", n / Pi);
    else if (n < Pi*99.95)
        snprintf(buf, sizeof buf, "%.1fPi", n / Pi);
    else if (n < Pi*1023.5)
        snprintf(buf, sizeof buf, "%.0fPi", n / Pi);

    else if (n < Ei*9.995)
        snprintf(buf, sizeof buf, "%.2fEi", n / Ei );
    else
        snprintf(buf, sizeof buf, "%.1fEi", n / Ei );
    return buf;
}

}   // namespace myserver