/**
* @description: LogStream.h
* @author: YQ Huang
* @brief: LogStream类
* @date: 2022/05/01 16:01:21
*/

#pragma once

#include "server/base/noncopyable.h"
#include "server/base/StringPiece.h"
#include <assert.h>
#include <string.h> // memcpy
#include <string>

namespace myserver {

// detail封装具体实现细节
namespace detail {

const int kSmallBuffer = 4000;  // 4K
const int kLargeBuffer = 4000 * 1000;   // 4M

/**
 * FixedBuffer 缓冲区 为一个非类型参数的模板类
 * SIZE表示缓冲区的大小
 */
template<int SIZE>
class FixedBuffer : noncopyable {
public:
    FixedBuffer()
        : cur_(data_)
    {
        setCookie(cookieStart);
    }

    ~FixedBuffer() {
        setCookie(cookieEnd);
    }

    // 将日志消息复制到缓冲区里
    void append(const char* buf, size_t len) {
        if(static_cast<size_t>(avail()) > len) {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    // 返回缓冲区首地址
    const char* data() const { return data_; }
    // 返回缓冲区当前已使用的空间
    int length() const { return static_cast<int>(cur_ - data_); }

    // 返回cur指针的地址
    char* current() { return cur_; }
    // 返回缓冲区当前可使用的空间
    int avail() const { return static_cast<int>(end() - cur_); }
    // 移动cur指针
    void add(size_t len) {cur_ += len; }

    // 重设缓冲区
    void reset() { cur_ = data_; }
    // 缓冲区全设为0
    void bzero() { memset(data_, 0, sizeof data_);}

    // 用于调式的
    const char* debugString();
    void setCookie(void (*cookie)()) { cookie_ = cookie; }
    // 用于单元测试的
    std::string toString() const { return std::string(data_, length()); }
    StringPiece toStringPiece() const { return StringPiece(data_, length()); }

private:
    const char* end() const { return data_ + sizeof data_; }    // 返回缓冲区的末地址
    static void cookieStart();  // 这两个是空操作
    static void cookieEnd();

    void (*cookie_)();  // 函数指针
    char data_[SIZE];   // data_指向缓冲区的首地址
    char* cur_; // cur_指向缓冲区中当前操作的位置
};

} // namespace detail

/**
 * 没有使用标准库的iostream，而是自己写的LogStream类，主要是出于性能 见书11.6.6
 * LogStream类把输出保存在自己内部的缓冲区 也就是FixedBuffer
 */
class LogStream : noncopyable {
public:
    // 大小为4KB的缓冲区
    typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;

    LogStream& operator<<(bool v) {
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }

    // 整型数据的字符串转换、保存到缓冲区；内部均调用formatInteger<T>函数
    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    // 指针类型转换为16进制字符串
    LogStream& operator<<(const void*);

    // 浮点类型数据转换为字符串；内部使用snprintf函数
    LogStream& operator<<(float v) {
        *this << static_cast<double>(v);
        return *this;
    }

    LogStream& operator<<(double);

    LogStream& operator<<(char v) {
        buffer_.append(&v, 1);
        return *this;
    }

    // 原生字符串输出到缓冲区
    LogStream& operator<<(const char* str) {
        if(str) {
            buffer_.append(str, strlen(str));
        }
        else {
            buffer_.append("(null)", 6);
        }
        return *this;
    }

    LogStream& operator<<(const unsigned char* str) {
        return operator<<(reinterpret_cast<const char*>(str));
    }

    // 标准字符串std::string输出到缓冲区
    LogStream& operator<<(const std::string& v) {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }

    // StringPiece输出到缓冲区
    LogStream& operator<<(const StringPiece& v) {
        buffer_.append(v.data(), v.size());
        return *this;
    }

    // Buffer输出到缓冲区
    LogStream& operator<<(const Buffer& v) {
        *this << v.toStringPiece();
        return *this;
    }

    // 将内容输出到缓冲区
    void append(const char* data, int len) { buffer_.append(data, len); }
    // 返回缓冲区
    const Buffer& buffer() const { return buffer_; }
    // 重设缓冲区
    void resetBuffer() { buffer_.reset(); }

private:
    void staticCheck(); // 静态检查 用于检查一些类型的大小

    template<typename T>
    void formatInteger(T);  // 将T类型转为10进制

    Buffer buffer_; // 缓冲区
    
    static const int kMaxNumericSize = 48;  // 用作检查类型大小的变量
};

} // namespace myserver