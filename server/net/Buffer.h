/**
* @description: Buffer.h
* @author: YQ Huang
* @brief: 输入输出缓冲区
* @date: 2022/06/19 17:07:06
*/

#pragma once

#include "server/base/copyable.h"
#include "server/base/StringPiece.h"
#include "server/base/Types.h"

#include <algorithm>
#include <vector>
#include <iostream>
#include <assert.h>
#include <string.h>
#include <endian.h>

namespace myserver {

namespace net {

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

/**
 * 输入输出缓冲区 
 */
class Buffer : public myserver::copyable {
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    // 构造函数
    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == initialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

    // 交换
    void swap(Buffer& rhs) {
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }

    // 返回可读字节数
    size_t readableBytes() const 
    { return writerIndex_ - readerIndex_; }

    // 返回可写字节数
    size_t writableBytes() const
    { return buffer_.size() - writerIndex_; }

    // 返回目前头部的可写字节数
    // 有时候经过若干次读写，readerIndex_移到了比较靠后的位置
    // 留下了巨大的prependable空间
    size_t prependableBytes() const
    { return readerIndex_; }

    // 返回数据可读处的首地址
    const char* peek() const
    { return begin() + readerIndex_; }

    // 从可写区域返回第一个\r\n的位置
    const char* findCRLF() const {
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    // 指定开始位置，从可写区域返回第一个\r\n的位置
    const char* findCRLF(const char* start) const {
        assert(peek() <= start);
        assert(start <= beginWrite());

        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    // 从可读区域返回第一个\n的位置
    const char* findEOL() const {
        const void* eol = memchr(peek(), '\n', readableBytes());
        return static_cast<const char*>(eol);
    }

    // 指定开始位置，从可读区域返回第一个\n的位置
    const char* findEOL(const char* start) const {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const void* eol = memchr(peek(), '\n', readableBytes());
        return static_cast<const char*>(eol);
    }

    /**
     * @brief retrieve类函数主要是操作指针移动 
     */

    // 从buffer里读走len个字节的数据
    void retrieve(size_t len) {
        assert(len <= readableBytes());
        // 如果可读字节大于要读的字节，那么直接将readIndex_指针向后移动len个位置
        if(len < readableBytes()) {
            readerIndex_ += len;
        }
        // 若len大于可读的字节数，那么直接读取完所有的字节
        else {
            retrieveAll();
        }
    }

    // 从buffer里读取指定结束位置的数据
    void retrieveUntil(const char* end) {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    // 从buffer里读取8字节数据
    void retrieveInt64() {
        retrieve(sizeof(int64_t));
    }

    // 从buffer里读取4字节数据
    void retrieveInt32() {
        retrieve(sizeof(int32_t));
    }

    // 从buffer里读取2字节数据
    void retrieveInt16() {
        retrieve(sizeof(int16_t));
    }

    // 从buffer里读取1字节数据
    void retrieveInt8() {
        retrieve(sizeof(int8_t));
    }

    // 从buffer里读取所有数据
    void retrieveAll() {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    // 读取所有的可读数据为 String 类型
    string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    // 将读取的数据转为 String 类型
    string retrieveAsString(size_t len) {
        assert(len <= readableBytes());
        string result(peek(), len);
        retrieve(len);
        return result;
    }

    // 读取所有的可读数据为 StringPiece 类型
    StringPiece toStringPiece() const {
        return StringPiece(peek(), static_cast<int>(readableBytes()));
    }

    // 添加 StringPiece 类型的数据
    void append(const StringPiece& str) {
        append(str.data(), str.size());
    }

    // 添加 char* 数据
    void append(const char* data, size_t len) {
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());
        hasWritten(len);
    }

    // 添加 void* 数据 
    void append(const void* data, size_t len) {
        append(static_cast<const char*>(data), len);
    }

    // 确保有足够的可写空间
    void ensureWritableBytes(size_t len) {
        if(writableBytes() < len) {
            // 如果空间不够，调用私有成员函数makeSpace增加可写空间
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    // 返回可写区域的首地址
    char* beginWrite() { return begin() + writerIndex_;}

    const char* beginWrite() const { return begin() + writerIndex_; }

    // 有 len 个长度已写
    void hasWritten(size_t len) {
        assert(len <= writableBytes());
        writerIndex_ += len;
    }

    // 有 len 个长度未写
    void unwrite(size_t len) {
        assert(len <= readableBytes());
        writerIndex_ -= len;
    }

    // 将 int*_t 类型的数据转换为网络字节序（大端） 写入缓冲区 
    void appendInt64(int64_t x) {
        int64_t be64 = ::htobe64(x);
        append(&be64, sizeof be64);
    }

    void appendInt32(int32_t x) {
        int32_t be32 = htobe32(x);
        append(&be32, sizeof be32);
    }

    void appendInt16(int16_t x) {
        int16_t be16 = htobe16(x);
        append(&be16, sizeof be16);
    }

    void appendInt8(int8_t x) {
        append(&x, sizeof x);
    }

    // 从缓冲区中读取 int*_t 类型的数据
    int64_t readInt64() {
        int64_t result = peekInt64();
        retrieveInt64();
        return result;
    }

    int32_t readInt32() {
        int32_t result = peekInt32();
        retrieveInt32();
        return result;
    }

    int16_t readInt16() {
        int16_t result = peekInt16();
        retrieveInt16();
        return result;
    }

    int8_t readInt8() {
        int8_t result = peekInt8();
        retrieveInt8();
        return result;
    }

    // 从缓冲区中读取 int*_t 大小的数据 并转为主机字节序
    int64_t peekInt64() const {
        assert(readableBytes() >= sizeof(int64_t));
        int64_t be64 = 0;
        ::memcpy(&be64, peek(), sizeof(be64));
        return ::be64toh(be64);
    }

    int32_t peekInt32() const {
        assert(readableBytes() >= sizeof(int32_t));
        int32_t be32 = 0;
        ::memcpy(&be32, peek(), sizeof(be32));
        return ::be32toh(be32);
    }

    int16_t peekInt16() const {
        assert(readableBytes() >= sizeof(int16_t));
        int16_t be16 = 0;
        ::memcpy(&be16, peek(), sizeof(be16));
        return ::be16toh(be16);
    }

    int8_t peekInt8() const {
        assert(readableBytes() >= sizeof(int8_t));
        int8_t x = *peek();
        return x;
    }

    // prepend类函数 让程序能以很低的代价在数据前面添加几个字节
    void prependInt64(int64_t x) {
        int64_t be64 = ::htobe64(x);
        prepend(&be64, sizeof be64);
    }

    void prependInt32(int32_t x) {
        int32_t be32 = ::htobe32(x);
        prepend(&be32, sizeof be32);
    }

    void prependInt16(int16_t x) {
        int16_t be16 = ::htobe16(x);
        prepend(&be16, sizeof be16);
    }

    void prependInt8(int8_t x) { 
        prepend(&x, sizeof x);
    }

    void prepend(const void* data, size_t len) {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d+len, begin()+readerIndex_);
    }

    // 收缩空间
    void shrink(size_t reserve) {
        Buffer other;
        other.ensureWritableBytes(readableBytes() + reserve);
        other.append(toStringPiece());
        swap(other);
    }

    // 返回 vector 当前分配的存储容量
    size_t inertnalCapacity() const {
        return buffer_.capacity();
    }

    // 直接读取数据到缓冲区
    ssize_t readFd(int fd, int* savedErrno);


private:
    // 返回缓冲区里的第一个字节的指针
    char* begin() { return &*buffer_.begin(); }
    const char* begin() const { return &*buffer_.begin(); }

    // 重新分配空间
    void makeSpace(size_t len) {
        // 如果头部剩下的空间和可写区间 小于 新加的数据长度和预留默认头部长度时
        // 利用vector.resize() 重新分配空间
        if(writableBytes() + prependableBytes() < len + kCheapPrepend) {
            buffer_.resize(writerIndex_ + len);
        }
        // 如果空间还足够，则只需内部腾挪
        else {
            assert(kCheapPrepend < readerIndex_);
            size_t readable = readableBytes();
            // 把数据移到前面去
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + kCheapPrepend);
            // 重新设置指针
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == readableBytes());
        }
    }

private:
    std::vector<char> buffer_;
    
    // 由于vector重新分配了内存，原来指向其元素的指针会失效
    // 所以readerIndex_和writerIndex_是整数下标而不是指针
    size_t readerIndex_;
    size_t writerIndex_;

    static const char kCRLF[];
};

}   // namespace net

}   // namespace myserver
