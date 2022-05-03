/**
* @description: CountDownLatch.h
* @author: YQ Huang
* @brief: 倒计时
* @date: 2022/05/03 20:22:00
*/

#pragma once

#include "server/base/Condition.h"
#include "server/base/Mutex.h"

namespace myserver {

/**
 * 倒计时类 用于同步 使用Condition实现
 * 
 * 设定一个需要准备就绪事件的数目数量，该变量被互斥保护，在其他线程中被修改，某线程阻塞等待
 * 直到变量减到为0，条件变量通知唤醒阻塞线程
 * 
 * 1) 主线程发起多个子线程 然后等待多个子线程完成初始化后才继续执行 子线程调用
 * 2) 主线程发起多个子线程 子线程等待主线程发出命令后才开始执行    主线程调用
 */
class CountDownLatch : noncopyable {
public:
    // 构造函数
    explicit CountDownLatch(int count);

    // 计数大于0 阻塞等待
    void wait();
    // 计数-1 等于0的话唤醒线程
    void countDown();
    // 获取计数
    int getCount() const;

private:
    mutable MutexLock mutex_;   // mutable修饰的成员变量在const成员函数里也可以改变 getCount需要这个属性
    Condition condition_;       // 条件变量
    int count_;                 // 计数
};

}   // namespacea myserver