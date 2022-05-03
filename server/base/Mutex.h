/**
* @description: Mutex.h
* @author: YQ Huang
* @brief: 互斥器类
* @date: 2022/05/03 17:17:34
*/

#pragma once

#include "server/base/CurrentThread.h"
#include "server/base/noncopyable.h"

#include <assert.h>
#include <pthread.h>

// 宏 检查pthread函数的返回值 成功的话应当返回0
#ifdef CHECK_PTHREAD_RETURN_VALUE

#ifdef NDEBUG
__BEGIN_DECLS
extern void __assert_perror_fail (int errnum,
                                  const char *file,
                                  unsigned int line,
                                  const char *function)
    noexcept __attribute__ ((__noreturn__));
__END_DECLS
#endif

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       if (__builtin_expect(errnum != 0, 0))    \
                         __assert_perror_fail (errnum, __FILE__, __LINE__, __func__);})

#else

#define MCHECK(ret) ({ __typedef__ (ret) errnum = (ret);        \
                       assert(errnum == 0); (void) errnum;})

#endif

namespace myserver {

/**
 * 用RAII手法封装mutex的创建、销毁、加锁、解锁这四个操作
 */
class MutexLock : noncopyable {
public:
    // 构造函数 创建mutex 拥有锁的线程id初始化为0
    MutexLock()
        : holder_(0)
    {
        MCHECK(pthread_mutex_init(&mutex_, NULL));
    }

    // 析构函数 销毁mutex
    // 不要销毁一个已经加锁或正在被条件变量使用的mutex 
    ~MutexLock() {
        assert(holder_ == 0);
        MCHECK(pthread_mutex_destroy(&mutex_));
    }

    // 是不是当前线程加了锁 
    bool isLockedByThisThread() const {
        return holder_ == CurrentThread::tid();
    }

    // 断言 是被当前线程加了锁
    void assertLocked() const {
        assert(isLockedByThisThread());
    }

    // 加锁 并且holder_赋值为持有该锁的线程tid
    void lock() {
        MCHECK(pthread_mutex_lock(&mutex_));
        assignHolder();
    }

    // 解锁 holder_赋为0
    void unlock() {
        unassignHolder();
        MCHECK(pthread_mutex_unlock(&mutex_));
    }

    // 获取mutex对象
    pthread_mutex_t* getPthreadMutex() {
        return &mutex_;
    }

private:
    friend class Condition; // Condition 友元类
    
    /**
     * 构造函数给线程tid置0
     * 析构函数给线程tid赋当前线程的tid
     * 为什么？多个线程需要调用条件变量时，为了方便其他线程上锁调用p_c_w，需要给mutex的holder_置0
     * 条件满足后自动析构再置回原来的值
     * 这样isLockedByThisThread()不会报错
     */
    class UnassignGuard : noncopyable {
    public:
        explicit UnassignGuard(MutexLock& owner)
            : owner_(owner)
        {
            owner_.unassignHolder();
        }

        ~UnassignGuard() {
            owner_.assignHolder();
        }
    
    private:
        MutexLock& owner_;
    };

    // 线程tid置0
    void unassignHolder() {
        holder_ = 0;
    }

    // 分配线程tid
    void assignHolder() {
        holder_ = CurrentThread::tid();
    }

    pthread_mutex_t mutex_;
    pid_t holder_;
};

/**
 * MutexLock管理类
 * 用RAII手法实现构造时给互斥加锁 析构时解锁
 */
class MutexLockGuard : noncopyable {
public:
    // 构造函数 给互斥加锁
    explicit MutexLockGuard(MutexLock& mutex)
        : mutex_(mutex)
    {
        mutex.lock();
    }

    ~MutexLockGuard() {
        mutex_.unlock();
    }

private:
    MutexLock& mutex_;
};

// 防止程序里出现临时对象MutexLockGuard 要加变量名！
#define MutexLockGuard(x) error "Missing guard object name"

}   // namespace myserver