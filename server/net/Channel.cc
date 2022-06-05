/**
* @description: Channel.cc
* @author: YQ Huang
* @brief: 对fd事件相关方法的封装
* @date: 2022/05/14 13:22:55
*/

#include "server/net/Channel.h"

#include "server/base/Logging.h"
#include "server/net/EventLoop.h"

#include <sstream>
#include <poll.h>

namespace myserver {

namespace net {

const int Channel::kNoneEvent = 0;
const int Channel::KReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

// 构造函数
Channel::Channel(EventLoop* loop, int fd__)
    : loop_(loop),
      fd_(fd__),
      events_(0),
      revents_(0),
      index_(-1),
      logHup_(true),
      tied_(false),
      eventHandling_(false),
      addedToLoop_(false)
{

}

// 析构函数
Channel::~Channel() {
    assert(!eventHandling_);
    assert(!addedToLoop_);
    if(loop_->isInLoopThread()) {
        assert(!loop_->hasChannel(this));
    }
}

// 生存期控制
void Channel::tie(const std::shared_ptr<void>& obj) {
    tie_ = obj;
    tied_ = true;
}

/**
 * 事件分发 由EventLoop::loop()调用
 * 内部调用线程安全的handleEventWithGuard()
 */ 
void Channel::handleEvent(Timestamp receiveTime) {
    std::shared_ptr<void> guard;
    if(tied_) {
        guard = tie_.lock();
        if(guard) {
            handleEventWithGuard(receiveTime);
        }
    }
    else {
        handleEventWithGuard(receiveTime);
    }
    
}

string Channel::reventsToString() const {
    return eventsToString(fd_, revents_);
}

string Channel::eventsToString() const {
    return eventsToString(fd_, events_);
}

// 关闭fd上注册的事件 并从Poll中移除
void Channel::remove() {
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

string Channel::eventsToString(int fd, int ev) {
    std::ostringstream oss;
    oss << fd << ": ";
    if(ev & POLLIN)
        oss << "IN ";
    if(ev & POLLPRI)
        oss << "PRI ";
    if(ev & POLLOUT)
        oss << "OUT ";
    if(ev & POLLHUP)
        oss << "HUP ";
    if(ev & POLLRDHUP)
        oss << "RDHUP ";
    if(ev & POLLERR)
        oss << "ERR ";
    if(ev & POLLNVAL)
        oss << "NVAL ";

    return oss.str();
}

// 把当前的channel加入到poll队列当中，或者更新fd的监听事件
void Channel::update() {
    addedToLoop_ = true;
    loop_->updateChannel(this);
}

/**
 * 查看Poll返回的具体是什么事件，并根据事件的类型进行相应的处理 
 */
void Channel::handleEventWithGuard(Timestamp reveiveTime) {
    eventHandling_ = true;
    LOG_TRACE << reventsToString();

    // 当事件为挂起并没有可读事件时
    if((revents_ & POLLHUP) &&  !(revents_ & POLLIN)) {
        if(logHup_) {
            LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
        }
        if(closeCallback_) {
            closeCallback_();
        }
    }

    // 描述符不是一个可以打开的文件描述符
    if(revents_ & POLLNVAL) {
        LOG_WARN << "fd = " << fd_ << "Channel::handle_event() POLLNVAL";
    }

    // 发生错误或者描述符不可打开
    if(revents_ & (POLLERR | POLLNVAL)) {
        if(errorCallback_) {
            errorCallback_();
        }
    }

    // 关于读的事件
    if(revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if(readCallback_) {
            readCallback_(reveiveTime);
        }
    }

    // 关于写的事件
    if(revents_ & POLLOUT) {
        if(writeCallback_) {
            writeCallback_();
        }
    }
    eventHandling_ = false;
}


}   // namespace net

}   // namespace myserver