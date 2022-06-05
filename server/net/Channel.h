/**
* @description: Channel.h
* @author: YQ Huang
* @brief: 对fd事件相关方法的封装
* @date: 2022/05/14 13:22:51
*/

#pragma once

#include "server/base/noncopyable.h"
#include "server/base/Timestamp.h"
#include "server/base/Types.h"

#include <functional>
#include <memory>

namespace myserver {

namespace net {

class EventLoop;

/**
 * Channel对象自始至终只负责一个文件描述符fd的IO事件分发，但它并不拥有这个fd
 * 也不会在析构的时候关闭这个fd
 * Channel会把不同的IO事件分发为不同的回调
 * 用户无须继承Channel，Channel不是基类。Channel的生命期由其owner class负责管理，它一般是其他class
 * 的直接或间接成员
 */
class Channel : noncopyable {
public:
   
    typedef std::function<void()> EventCallback;                // 事件回调函数定义
    typedef std::function<void(Timestamp)> ReadEventCallback;   // 读事件回调函数定义

    // 构造函数
    Channel(EventLoop* loop, int fd);
    // 析构函数
    ~Channel();

    // 处理事件
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 生存期控制
    void tie(const std::shared_ptr<void>&);

    // 返回文件描述符
    int fd() const { return fd_; }
    // 返回注册的事件
    int events() const { return events_; }
    // 设置就绪的事件
    void set_revents(int revt) { revents_ = revt; }
    // 判断是否注册的事件
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    
    void enableReading() { events_ |= KReadEvent; update(); }   // 注册可读事件
    void disableReading() { events_ &= ~KReadEvent; update(); } // 取消注册可读事件
    void enableWriting() { events_ |= kWriteEvent; update(); }  // 注册可写事件
    void disableWriting() { events_ &= ~kWriteEvent; update(); } // 取消注册可写事件
    void disableAll() { events_ = kNoneEvent; update(); }        // 全部取消注册
    bool isWriting() const { return events_ & kWriteEvent; }     // 判断是否可读事件
    bool isReading() const { return events_ & KReadEvent; }      // 判断是否可写事件

    // 获取Poller使用的下标
    int index() { return index_; }
    // 设置Poller使用的下标
    void set_index(int idx) { index_ = idx; }

    // for debug
    // 事件转换为字符串，方便打印调试
    string reventsToString() const;
    string eventsToString() const;

    void doNotLogHup() { logHup_ = false; }

    // 返回所属的EventLoop
    EventLoop* ownerLoop() { return loop_; }
    // 关闭fd上注册的事件 并从Poll中移除
    void remove();

private:
    static string eventsToString(int fd, int ev);

    void update();  // 把当前的channel加入到poll队列当中，或者更新fd的监听事件
    void handleEventWithGuard(Timestamp reveiveTime);   // 加锁的事件处理

    static const int kNoneEvent;    // 无事件
    static const int KReadEvent;    // 可读事件
    static const int kWriteEvent;   // 可写事件

    EventLoop* loop_;               // channel所属的loop
    const int fd_;                  // channel负责的文件描述符
    int events_;                    // 注册的IO事件
    int revents_;                   // 目前活动的事件 由EventLoop/Poller设置
    int index_;                     // 被Poller使用的下标
    bool logHup_;                   // 是否生成某些日志

    std::weak_ptr<void> tie_;       // 负责生存期控制
    bool tied_;
    bool eventHandling_;            // 是否在处理事件
    bool addedToLoop_;              // 是否已经加入poll
    ReadEventCallback readCallback_;    // 读事件回调函数
    EventCallback writeCallback_;       // 写事件回调函数
    EventCallback closeCallback_;       // 关闭事件回调函数
    EventCallback errorCallback_;       // 错误事件回调函数

};

}   // namespace net

}   // namespace myserver