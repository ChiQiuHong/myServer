/**
* @description: Poller.cc
* @author: YQ Huang
* @brief: IO复用的封装
* @date: 2022/05/14 19:41:16
*/

#include "server/net/Poller.h"

#include "server/net/Channel.h"

namespace myserver {

namespace net {

Poller::Poller(EventLoop* loop)
    : ownerLoop_(loop)
{

}

Poller::~Poller() = default;


}   // namespace net
    
}   // namespace myserver