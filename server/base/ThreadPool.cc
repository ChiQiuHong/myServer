/**
* @description: ThreadPool.cc
* @author: YQ Huang
* @brief: 线程池
* @date: 2022/05/04 21:29:53
*/

#include "server/base/ThreadPool.h"

#include <assert.h>
#include <stdio.h>

namespace myserver {

// 构造函数 只初始化参数
ThreadPool::ThreadPool(const string& nameArg)
    : mutex_(),
      notEmpty_(mutex_),    // 初始化条件变量
      notFull_(mutex_),
      name_(nameArg),   // 初始化线程池名称
      maxQueueSize_(0), // 任务列表最大值初始化为0
      running_(false)
{
}

// 析构函数
ThreadPool::~ThreadPool() {
    // 如果线程池在运行，则停止线程池
    if(running_) {
        stop();
    }
    // 如果没有分配过线程，那就也没有需要释放的内存 什么都不做就可以了
}

// 创建numThreads个线程
void ThreadPool::start(int numThreads) {
    running_ = true;
    threads_.reserve(numThreads);
    for(int i = 0; i < numThreads; ++i) {
        char id[32];
        snprintf(id, sizeof(id), "%d", i+1);
        // std::bind在绑定类内部成员时，第二个参数必须是类的实例
        threads_.emplace_back(new Thread(std::bind(&ThreadPool::runInThread, this), name_+id));
        // 启动每个线程，但是由于线程运行函数是runInThread 所以会阻塞
        threads_[i]->start();
    }
    // 如果线程池线程数为0 且设置了回调函数 则调用回调函数
    if(numThreads == 0 && threadInitCallback_) {
        threadInitCallback_();
    }
}

// 关闭线程池
void ThreadPool::stop() {
    {
        MutexLockGuard lock(mutex_);    // 局部加锁
        running_ = false;
        notEmpty_.notifyAll();      // 唤醒等待的所有线程
        notFull_.notifyAll();
    }
    // 对所有线程调用join
    for(auto& thr : threads_) {
        thr->join();
    }
}

// 返回任务队列的大小
size_t ThreadPool::queueSize() const {
    MutexLockGuard lock(mutex_);
    return queue_.size();
}

// 生产者函数 负责生成任务
void ThreadPool::run(Task task) {
    // 如果线程池为空 即没有分配线程 直接有当前线程执行任务
    if(threads_.empty()) {
        task();
    }
    else {
        MutexLockGuard lock(mutex_);
        // 如果任务队列满了，则阻塞等待 直到队列不满
        while(isFull() && running_) {
            notFull_.wait();
        }
        // 如果线程池关闭了 直接返回
        if(!running_) return;

        // 当任务队列没满 则将任务加入队列中
        queue_.push_back(std::move(task));
        // 唤醒等待取任务的线程
        notEmpty_.notify();
    }
}

// 判断任务队列是否满了
bool ThreadPool::isFull() const {
    mutex_.assertLocked();
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

// 消费者函数 从任务队列中取任务task
void ThreadPool::runInThread() {
    try {
        if(threadInitCallback_) {
            // 支持每个线程运行前执行初始化
            threadInitCallback_();  
        }
        // 线程池启动后 该线程就一直循环
        while(running_) {
            // 从任务列表中取任务 执行
            Task task(take());
            if(task) {
                task();
            }
        }
    }
    catch(const std::exception& ex) {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }
    catch(...) {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw;
    }
}

// 从任务队列中取函数
ThreadPool::Task ThreadPool::take() {
    MutexLockGuard lock(mutex_);
    // 如果任务队列为空 则线程阻塞 直到有任务才唤醒
    while(queue_.empty() && running_) {
        notEmpty_.wait();
    }

    Task task;
    // 再判断一下是否非空
    if(!queue_.empty()) {
        // 总是从队列队头取任务
        task = queue_.front();
        if(maxQueueSize_ > 0) {
            // 已经取走一个任务，唤醒等待放任务的线程
            notFull_.notify();
        }
    }

    return task;
}

}   // namespaace myserver