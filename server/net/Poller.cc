/**
* @description: Poller.cc
* @author: YQ Huang
* @brief: IO复用的封装
* @date: 2022/05/14 19:41:16
*/

#include "server/net/Poller.h"

#include "server/base/Logging.h"
#include "server/net/Channel.h"

#include <assert.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace myserver {

namespace net {

const int kNew = -1;    // 新增
const int kAdded = 1;   // 已添加
const int kDeleted = 2; // 已删除

/**
 * 构造函数
 * 调用epoll_create1创建epoll例程epoll
 * 创建成功返回文件描述符，失败返回-1
 */ 
Poller::Poller(EventLoop* loop)
    : ownerLoop_(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    if(epollfd_ < 0) {
        // 创建失败 终止程序 
        LOG_SYSFATAL << "Poller::Poller";
    }
}

// 析构函数 关闭epoll文件描述符
Poller::~Poller() {
    ::close(epollfd_);
}

/**
 * 调用epoll获得当前活动的IO事件 然后填充调用方传入的activeChannels
 * 并返回epoll return的时刻
 * @param activeChannels 调用方传入的Channel
 * @param timeoutMs 以 1/1000秒为单位的等待时间，传递-1时，一直等待直到发生事件
 */
Timestamp Poller::poll(int timeoutMs, ChannelList* activeChannels) {
    LOG_TRACE << "fd total count " << channels_.size();
    // epoll_wait() 和 select()类似 等待文件描述符发生变化
    // &*events_.begin() 是获得元素的首地址 等价于 events_.data()
    // 成功时返回发生事件的文件描述符数 失败时返回-1
    int numEvents = ::epoll_wait(epollfd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);

    int savedErrno = errno;
    // epoll_wait() 返回的时刻
    Timestamp now(Timestamp::now());
    if(numEvents > 0) {
        LOG_TRACE << numEvents << " events happened";
        // 将活动事件填充进activeChannels
        fillActiveChannels(numEvents, activeChannels);
        // 调整events_数组的空间
        if(implicit_cast<size_t>(numEvents) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0) {
        LOG_TRACE << "nothing happend";
    }
    else {
        if(savedErrno != EINTR) {
            errno = savedErrno;
            LOG_SYSERR << "Poller::poll()";
        }
    }
    return now;
}

/**
 * updateChannel()的主要功能是负责维护和更新存储Channel的map
 * 添加新Channel的复杂度是O(log N)
 * 更新已有的Channel的复杂度是O(1)
 */
void Poller::updateChannel(Channel* channel) {
    Poller::assertInLoopThread();
    const int index = channel->index();
    LOG_TRACE << "fd = " << channel->fd()
        << " events = " << channel->events() << " index = " << index;
    // 如果channel是新增的或是已经从epoll例程里删除了的
    if(index == kNew || index == kDeleted) {
        int fd = channel->fd();
        if(index == kNew) {
            assert(channels_.find(fd) == channels_.end());
            // 添加新Channel
            channels_[fd] = channel;
        }
        else {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }

        // channel设为kAdded状态 将fd注册到epoll例程
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else {
        int fd = channel->fd();
        (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);
        // 如果Channel暂时不关心任何事件了
        if(channel->isNoneEvent()) {
            // 从epoll例程中删除fd channel设为kDeleted状态
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        // 否则 修改已经注册的fd的事件 如可读或可写
        else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 移除Channel 时间复杂度O(log N)
void Poller::removeChannel(Channel* channel) {
    Poller::assertInLoopThread();
    int fd = channel->fd();
    LOG_TRACE << "fd = " << fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    // 移除Channel
    size_t n = channels_.erase(fd);
    (void)n;
    assert(n == 1);

    // 这里是kAdded
    if(index == kAdded) {
        // 从epoll例程里删除channle对应的事件
        update(EPOLL_CTL_DEL, channel);
    }
    // 把channel设为 -1
    channel->set_index(kNew);
}

/**
 * 输出操作所对应的字符串
 */
const char* Poller::operationToString(int op) {
    switch (op)
    {
    case EPOLL_CTL_ADD:
        return "ADD";
    case EPOLL_CTL_DEL:
        return "DEL";
    case EPOLL_CTL_MOD:
        return "MOD";
    default:
        assert(false && "ERROR op");
        return "Unknown Operation";
    }
}

/**
 * 遍历events_, 把它对应的Channel填入activeChannels
 * 函数复杂度为 O(N), N 是 numEvents的大小
 * 
 * 注意这里我们没有一边遍历一边调用Channel::handleEvent()
 * 是简化Poller的职责，它只负责IO复用，
 * 
 * @param numEvents epoll_wait()返回的文件描述符数 
 */
void Poller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const {
    assert(implicit_cast<size_t>(numEvents) <= events_.size());
    for(int i = 0; i < numEvents; ++i) {
        // events_[i].data.ptr 在 Poller::update()里已经被设置为channel 
        // 这里需要一个强制转换 从void* 转为Channel*
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
        // 调试代码 测试 ChannelMap里是否有这个文件描述符
        int fd = channel->fd();
        ChannelMap::const_iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
#endif
        // channel 设置就绪的事件 供Channel::handleEvent()使用 以调用回调函数
        channel->set_revents(events_[i].events);
        // 填入activeChannels
        activeChannels->push_back(channel);
    }
}

/**
 * 注册/删除事件的核心操作 调用epoll_ctl() 函数
 * 
 * EPOLL_CTL_ADD 将文件描述符注册到epoll例程
 * EPOLL_CTL_DEL 从epoll例程中删除文件描述符
 * EPOLL_CTL_MOD 更改注册的文件描述符的关注事件发生情况
 */
void Poller::update(int operation, Channel* channel) {
    struct epoll_event event;
    memZero(&event, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    LOG_TRACE << "epoll_ctl op = " << operationToString(operation)
        << " fd = " << fd << " event = { " << channel->eventsToString() << " }";
    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if(operation == EPOLL_CTL_DEL) {
            LOG_SYSERR << "epoll_ctl op = " << operationToString(operation) << " fd = " << fd;
        }
        else {
            LOG_SYSFATAL << "epoll_ctl op = " << operationToString(operation) << " fd = " << fd;
        }
    }
}

}   // namespace net
    
}   // namespace myserver