/**
* @description: StringPiece.h
* @author: YQ Huang
* @brief: 字符串的一个代理类 是PCRE的实现
* @date: 2022/05/01 20:51:25
*/

// Taken from PCRE pcre_stringpiece.h
// Copyright (c) 2005, Google Inc.
// All rights reserved.
// Author: Sanjay Ghemawat

/**
 * StringPiece不涉及内存拷贝，实现了高效的字符串传递。
 * 比如形参为const std::string，实参为const char*时
 * 会分配内存并拷贝该字符串以生成一个std::string 性能开销大
 * 使用场景：目的仅仅是读取字符串的值
 */

#pragma once

#include <string>
#include <string.h>
#include <iosfwd> // for ostream forward-declaration

namespace myserver {

// 传递c类型的字符串
class StringArg {
public:
    StringArg(const char* str)
        : str_(str)
    { }

    StringArg(const std::string& str)
        : str_(str.c_str())
    { }

    const char* c_str() const { return str_; }

private:
    const char* str_;
};

/**
 * StringPiece对string/char*进行封装，封装成const char* ptr_ + int length_
 * 这样StringPiece类的大小始终都是4字节 + 4字节
 * 通过保存字符串指针和长度，来避免不必要的复制
 */
class StringPiece {
public:
    // 构造函数
    StringPiece()
        : ptr_(NULL), length_(0) { }

    StringPiece(const char* str)
        : ptr_(str), length_(static_cast<int>(strlen(ptr_))) { }

    StringPiece(const unsigned char* str)
        : ptr_(reinterpret_cast<const char*>(str)),
          length_(static_cast<int>(strlen(ptr_))) { }
    
    StringPiece(const std::string& str)
        : ptr_(str.data()), length_(static_cast<int>(str.size())) { }

    StringPiece(const char* offset, int len)
        : ptr_(offset), length_(len) { }

    // 操作类函数
    const char* data() const { return ptr_; }
    int size() const { return length_; }
    bool empty() const { return length_ == 0; }
    const char* begin() const { return ptr_; }
    const char* end() const { return ptr_ + length_; }

    void clear() { ptr_ = NULL; length_ = 0; }
    void set(const char* buffer, int len) { ptr_ = buffer; length_ = len; }
    void set(const char* str) {
        ptr_ = str;
        length_ = static_cast<int>(strlen(str));
    }
    void set(const void* buffer, int len) {
        ptr_ = reinterpret_cast<const char*>(buffer);
        length_ = len;
    }

    char operator[](int i) const { return ptr_[i]; }

    // 移除ptr_的前n个字节
    void remove_prefix(int n) {
        ptr_ += n;
        length_ -= n;
    }
    // 移除ptr_的后n个字节
    void remove_suffix(int n) {
        length_ -= n;
    }

    // 字符串比较
    bool operator==(const StringPiece& x) const {
        return ((length_ == x.length_) &&
                (memcmp(ptr_, x.ptr_, length_) == 0));
    }

    bool operator!=(const StringPiece& x) const {
        return !(*this == x);
    }

// 比较”abcd” < “abcdefg”, 返回的结果为true
// 比较”abcdx” < “abcdefg”, 返回结果为false
#define STRINGPIECE_BINARY_PREDICATE(cmp, auxcmp)                                \
    bool operator cmp (const StringPiece& x) const {                             \
        int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_); \
        return ((r auxcmp 0) || ((r == 0) && (length_ cmp x.length_)));          \
    }
    STRINGPIECE_BINARY_PREDICATE(<, <);
    STRINGPIECE_BINARY_PREDICATE(<=, <);
    STRINGPIECE_BINARY_PREDICATE(>=, >);
    STRINGPIECE_BINARY_PREDICATE(>, >);
#undef STRINGPIECE_BINARY_PREDICATE

    int compare(const StringPiece& x) const {
        int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
        if(r == 0) {
            if(length_ < x.length_)
                r = -1;
            else if(length_ > x.length_) r = +1;
        }
        return r;
    }

    std::string as_string() const {
        return std::string(data(), size());
    }

    void CopyToString(std::string* target) const {
        target->assign(ptr_, length_);
    }

    bool starts_with(const StringPiece& x) const {
        return ((length_ >= x.length_) && (memcmp(ptr_, x.ptr_, x.length_) == 0));
    }
    
private:
    const char* ptr_;   // 用于帮助字符串的读取
    int length_;    // 目的字符串的长度
};

/**
 * 由于StringPiece只持有目标指针，所以是POD类型，并且拥有平凡构造函数
 * 所以利用traits以指示STL采用更高级的算法实现和模板特化与偏特化能力，提高性能 
 */
#ifdef HAVE_TYPE_TRAITS
// This makes vector<StringPiece> really fast for some STL implementations
template<> struct __type_traits<muduo::StringPiece> {
  typedef __true_type    has_trivial_default_constructor;
  typedef __true_type    has_trivial_copy_constructor;
  typedef __true_type    has_trivial_assignment_operator;
  typedef __true_type    has_trivial_destructor;
  typedef __true_type    is_POD_type;
};
#endif

// allow StringPiece to be logged
std::ostream& operator<<(std::ostream& o, const myserver::StringPiece& piece);

}   // namespace myserver