/**
* @description: ThreadPool.h
* @author: YQ Huang
* @brief: 线程池
* @date: 2022/05/04 21:30:06
*/

#pragma once

#include "server/base/Condition.h"
#include "server/base/Mutex.h"
#include "server/base/Thread.h"
#include "server/base/Types.h"

#include <deque>
#include <vector>

namespace myserver {

/**
 * 线程池 提前分配一定数量的线程 依次从任务队列中拿出一个任务到线程中执行
 * 这里的线程池的线程数量是固定的，不能随运行时调整大小
 * 生产者消费者模型
 * 这里没有用BlockingQueue类来实现 而是用双端队列std::deque
 */
class ThreadPool : noncopyable {
public:
    typedef std::function<void()> Task; // 定义了一个任务 类型是函数指针
    
    // 构造函数 只初始化参数
    explicit ThreadPool(const string& nameArg = string("ThreadPool"));
    // 析构函数 关闭线程池
    ~ThreadPool();

    // Must be called before start()
    // 设定任务列表的最大值
    void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
    // 设定线程初始化的回调函数
    void setThreadInitCallback(const Task& cb) {
        threadInitCallback_ = cb;
    }

    // 创建线程池
    void start(int numThreads);
    // 关闭线程池
    void stop();

    // 返回线程池名称
    const string& name() const { return name_; }

    // 返回任务列表的大小
    size_t queueSize() const;

    // 生产者函数
    void run(Task f);

private:
    bool isFull() const;    // 判断任务队列是否满了
    void runInThread();     // 消费者函数 线程池中每个线程的运行函数（不断从队列中获取任务并执行）
    Task take();            // 从队列中取出一个任务

    mutable MutexLock mutex_;   // 与条件变量配合使用的互斥锁
    Condition notEmpty_;    // 条件变量 任务列表是否为空
    Condition notFull_;     // 条件变量 任务列表是否为满
    string name_;           // 线程池名称
    Task threadInitCallback_;   // 线程执行前的回调函数
    std::vector<std::unique_ptr<Thread> > threads_; // 存放线程指针
    std::deque<Task> queue_;    // 任务列表 线程安全的阻塞队列
    size_t maxQueueSize_;       // 任务列表最大数目
    bool running_;              // 线程池运行标志
};

}   // namespaace myserver