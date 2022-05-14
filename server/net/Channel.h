/**
* @description: Channel.h
* @author: YQ Huang
* @brief: 对fd事件相关方法的封装
* @date: 2022/05/14 13:22:51
*/

#pragma once

#include "server/base/noncopyable.h"
#include "server/base/Timestamp.h"

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
    // 回调函数
    typedef std::function<void()> EventCallback;

    // 构造函数
    Channel(EventLoop* loop, int fd);
    // 析构函数
    ~Channel();

    // 处理事件
    void handleEvent();

    // 设置回调函数
    void setReadCallback(const EventCallback& cb) { readCallback_ = cb; }
    void setWriteCallback(const EventCallback& cb) { writeCallback_ = cb; }
    void setErrorCallback(const EventCallback& cb) { errorCallback_ = cb; }

    // 返回文件描述符
    int fd() const { return fd_; }
    // 返回注册的事件
    int events() const { return events_; }
    // 设置就绪的事件
    void set_revents(int revt) { revents_ = revt; }
    // 判断是否注册的事件
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    // 注册可读事件
    void enableReading() { events_ |= KReadEvent; update(); }

    // 获取Poller使用的下标
    int index() { return index_; }
    // 设置Poller使用的下标
    void set_index(int idx) { index_ = idx; }

    EventLoop* ownerLoop() { return loop_; }

private:
    void update();

    static const int kNoneEvent;    // 无事件
    static const int KReadEvent;    // 可读事件
    static const int kWriteEvent;   // 可写事件

    EventLoop* loop_;               // channel所属的loop
    const int fd_;                  // channel负责的文件描述符
    int events_;                    // 注册的IO事件
    int revents_;                   // 目前活动的事件 由EventLoop/Poller设置
    int index_;                     // 被Poller使用的下标

    EventCallback readCallback_;    // 读事件回调函数
    EventCallback writeCallback_;   // 写事件回调函数
    EventCallback errorCallback_;   // 错误事件回调函数

};

}   // namespace net

}   // namespace myserver