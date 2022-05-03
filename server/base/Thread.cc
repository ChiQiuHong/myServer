/**
* @description: Thread.cc
* @author: YQ Huang
* @brief: 对pthread封装
* @date: 2022/05/03 10:06:38
*/

#include "server/base/Thread.h"
#include "server/base/CurrentThread.h"

#include "server/base/Logging.h"

#include <type_traits>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>


namespace myserver {

// detail 具体实现细节
namespace detail {

// 获取线程tid
pid_t gettid() {
    return static_cast<pid_t>(syscall(SYS_gettid));
}

// 线程名称初始化
class ThreadNameInitializer {
public:
    ThreadNameInitializer() {
        CurrentThread::t_threadName = "main";
        CurrentThread::tid();
        // pthread_arfork(NULL, NULL, &afterfork);
    }
};

ThreadNameInitializer init;

/**
 * 中间结构体ThreadDate 保存子线程的信息
 */
struct ThreadData {
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;   // 线程执行函数
    string name_;       // 子线程的名字
    pid_t* tid_;        // 子线程的tid
    // CountDownLatch* latch_; // FIXME

    // 构造函数
    ThreadData(ThreadFunc func,
               const string& name,
               pid_t* tid)
        : func_(std::move(func)),
          name_(name),
          tid_(tid)
    { }

    // 线程实际执行的函数
    void runInThread() {
        // 执行用户传入的回调函数
        *tid_ = CurrentThread::tid();
        tid_ = NULL;

        CurrentThread::t_threadName = name_.empty() ? "myserverThread" : name_.c_str();
        prctl(PR_SET_NAME, CurrentThread::t_threadName);
        try {
            func_();
            CurrentThread::t_threadName = "finished";
        }
        catch (const std::exception& ex) {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            abort();
        }
        catch (...) {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
            throw; // rethrow
        }
    }
};

// 因为pthread_create的线程函数定义为void* func(void*)
// 无法将non-static成员传递给pthread_create
// 所以startThread是一个跳板函数 调用ThreadData的runInThread
void* startThread(void* obj) {
    ThreadData* data = static_cast<ThreadData*>(obj);
    data->runInThread();
    delete data;
    return NULL;
}

}   // namespace detail

namespace CurrentThread {

void cacheTid() {
    if(t_cachedTid == 0) {
        t_cachedTid = detail::gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    }
}

bool isMainThread() {
    return tid() == getpid();
}

void sleepUsec(int64_t usec) {
    struct timespec ts = { 0, 0 };
    ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
    nanosleep(&ts, NULL);
}

}   // namespace CurrentThread

/**
 * 构造函数
 * 主要是对数据成员进行初始化 还没创建线程执行入口函数
 */
Thread::Thread(ThreadFunc func, const string& n)
    : started_(false),
      joined_(false),
      pthreadId_(0),
      tid_(0),
      func_(std::move(func)),
      name_(n)
{   
    // 线程名字默认为空
    setDefalutName();
}

/**
 * 析构函数
 */
Thread::~Thread() {
    // 保证线程已经开始运行并且还没join 才能进行detach
    if(started_ && !joined_) {
        pthread_detach(pthreadId_);
    }
}

/** 线程执行函数 // FIXME
 * 将数据封装在ThreadData类中，实际执行过程在ThreadData类的runInThread()成员函数中
 */ 
void Thread::start() {
    started_ = true;
    detail::ThreadData* data = new detail::ThreadData(func_, name_, &tid_);
    if(pthread_create(&pthreadId_, NULL, &detail::startThread, data)) {
        started_ = false;
        delete data;
        LOG_SYSFATAL << "Failed in pthread_create";
    }
}

// 等待线程执行完成 FIXME
int Thread::join() {
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}

// 设置默认线程名称
void Thread::setDefalutName() {
    if(name_.empty()) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Thread%d", 1);
        name_ = buf;
    }
}

}   // namespace myserver