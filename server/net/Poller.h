/**
* @description: Poller.h
* @author: YQ Huang
* @brief: IO复用的封装
* @date: 2022/05/14 19:41:08
*/

#pragma once

#include "server/base/Timestamp.h"
#include "server/net/EventLoop.h"

#include <map>
#include <vector>

namespace myserver {

namespace net {

class Channel;

/**
 * Poller是个抽象基类 同时支持poll和epoll两种IO复用机制
 * Poller是EventLoop的间接成员 只供其owner EventLoop在IO线程调用，因此无须加锁
 * 其生命期与EventLoop相等
 * Poller并不拥有Channel，Channel在析构之前必须自己unregister，避免空悬指针
 */
class Poller : noncopyable {
public:
    typedef std::vector<Channel*> ChannelList;

    Poller(EventLoop* loop);
    virtual ~Poller();

    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

    virtual void updateChannel(Channel* channel) = 0;

    void assertInLoopThread() const { 
        ownerLoop_->assertInLoopThread();
    }

protected:
    typedef std::map<int, Channel*> ChannelMap;

    ChannelMap channels_;

private:
    EventLoop* ownerLoop_;
};

}   // namespace net
    
}   // namespace myserver