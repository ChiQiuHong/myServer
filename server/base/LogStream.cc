/**
* @description: LogStream.cc
* @author: YQ Huang
* @brief: LogStream类
* @date: 2022/05/01 16:01:25
*/

#include "server/base/LogStream.h"

#include <algorithm>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

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
}   // namespace myserver