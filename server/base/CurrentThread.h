/**
* @description: CurrentThread.h
* @author: YQ Huang
* @brief: 实现对线程id的管理和优化
* @date: 2022/05/03 14:11:35
*/

#pragma once

#include "server/base/Types.h"

namespace myserver {

// CurrentThread命名空间
namespace CurrentThread {

// __thread关键字 表示 线程局部存储 即每一个线程内有一份独立实体 线程间互不干扰
// 可以用来修饰那些带有全局性且值可能变，但是又不值得用全局变量保护的变量
// 存取效率可以与全局变量相比 
// 这里缓存了线程id、id字符串用于日志中，避免了每次输出log都要调用一次
// 系统调用来获取线程id 再转换成字符串的操作，从而提高了效率
extern __thread int t_cachedTid;
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char* t_threadName;
void cacheTid();

inline int tid() {
    // 进行分支预测优化 避免跳转语句
    // 这里t_cachedTid == 0仅第一次满足，后面都不再执行if的代码块了
    if(__builtin_expect(t_cachedTid == 0, 0)) {
        cacheTid();
    }
    return t_cachedTid;
}

inline const char* tidString() {
    return t_tidString;
}

inline int tidStringLength() {
    return t_tidStringLength;
}

inline const char* name() {
    return t_threadName;
}

// 下面两个函数的具体实现在 Thread.cc
bool isMainThread();
void sleepUsec(int64_t usec);   // for testing

// 用在错误后定位错误信息 与 exception 结合使用
string stackTrace(bool demangle);

}   // namespace CurrentThread

}   // namespace myserver