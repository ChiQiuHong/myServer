/**
* @description: Atomic.h
* @author: YQ Huang
* @brief: 原子操作类
* @date: 2022/05/04 08:53:08
*/

#pragma once

#include "server/base/noncopyable.h"

#include <stdint.h>



namespace myserver {

namespace detail {

/**
 * 原子操作类 模板类
 * 所谓原子操作 就是不会被线程调度机制打断的操作，这种操作一旦开始就一直运行到结束。
 * 如自加自减赋值操作都不是原子操作 就需要用锁来保证操作的安全性
 * 使用GCC提供的系列build-in函数，用于提供加减和逻辑运算的原子操作
 */
template<typename T>
class AtomcIntegerT : noncopyable {
public:
    // 构造函数
    AtomcIntegerT()
        : value_(0)
    { }

    // 获取
    T get() {
        // __atomic__load_n 原子加载操作 返回value的内容
        return __atomic__load_n(&value_, __ATOMIC_SEQ_CST);
    }

    // 实现先获取值再修改值的操作
    T getAndAdd(T x) {
        // 返回未修改的value的值 然后value加上x
        return __atomic_fetch_add(&value_, x, __ATOMIC_SEQ_CST);
    }

    // 实现先修改值再获取值的操作
    T addAndGet(T x) {
        return getAndAdd(x) + x;
    }

    // ++n
    T incrementAndGet() {
        return addAndGet(1);
    }

    // --n
    T decrementAndGet() {
        return addAndGet(-1);
    }

    // 仅增加x 不返回
    void add(T x) {
        getAndAdd(x);
    }

    // 仅增加1 不返回
    void increment() {
        incrementAndGet();
    }

    // 仅减1 不返回
    void decrement() {
        decrementAndGet();
    }

    // 将value设为newValue 并返回value被操作之前的值
    T getAndSet(T newValue) {
        return __atomic_exchange_n(&value_, newValue, __ATOMIC_SEQ_CST);
    }

private:
    volatile T value_;  // volatile关键字 防止value_被编译器优化省略 要求每次直接读值
};

}   // namespace detail

// 实例化 主要定义了int32_t和int64_t类型的原子类型
// c++11中可使用std::atomic<int32_t/int64_t>
typedef detail::AtomcIntegerT<int32_t> AtomicInt32;
typedef detail::AtomcIntegerT<int64_t> AtomicInt64;

}   // namespace myserver