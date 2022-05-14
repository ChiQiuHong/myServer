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

const int kNoneEvent = 0;
const int kReadEvent = POLLIN | POLLPRI;
const int kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd__)
    : loop_(loop),
      fd_(fd__),
      events_(0),
      revents_(0),
      index_(-1)
{

}

Channel::~Channel() {

}

void Channel::handleEvent() {
    if(revents_ & POLLNVAL) {
        LOG_WARN << "Channel::handle_event() POLLNVAL";
    }

    if(revents_ & (POLLERR | POLLNVAL)) {
        if(errorCallback_) {
            errorCallback_();
        }
    }

    if(revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if(readCallback_) {
            readCallback_();
        }
    }

    if(revents_ & POLLOUT) {
        if(writeCallback_) {
            writeCallback_();
        }
    }
}

void Channel::update() {
    loop_->updateChannel(this);
}


}   // namespace net

}   // namespace myserver