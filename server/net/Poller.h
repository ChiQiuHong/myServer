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
 * Poller是个具体类 使用 epoll IO复用机制
 * Poller是EventLoop的间接成员 只供其owner EventLoop在IO线程调用，因此无须加锁
 * 其生命期与EventLoop相等
 * Poller并不拥有Channel，Channel在析构之前必须自己unregister，避免空悬指针
 */
class Poller : noncopyable {
public:
    typedef std::vector<Channel*> ChannelList;  // Channel数组

    // 构造函数
    Poller(EventLoop* loop);
    // 析构函数
    ~Poller();

    // 调用epoll获得当前活动的IO事件 然后填充调用方传入的activeChannels
    // 并返回poll return的时刻
    Timestamp poll(int timeoutMs, ChannelList* activeChannels);

    // 维护更新Channel
    void updateChannel(Channel* channel);
    // 移除Channel
    void removeChannel(Channel* channel);

    // 是否是当前线程的EventLoop调用的Poller
    void assertInLoopThread() const { 
        ownerLoop_->assertInLoopThread();
    }

private:
    typedef std::map<int, Channel*> ChannelMap;         // fd 到 Channel* 的映射
    typedef std::vector<struct epoll_event> EventList;  // epoll_event结构体数组

    static const int kInitEventListSize = 16;           // 初始化events_的大小

    // 输出操作所对应的字符串
    static const char* operationToString(int op);

    // 遍历events_，把它对应的Channel填入activeChannels
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    // 更新
    void update(int operation, Channel* channel);

    ChannelMap channels_;   // 存储事件分发器的map
    EventLoop* ownerLoop_;  // 调用方
    EventList events_;      // 传递给epoll_wait()时 发生变化的文件描述符信息将被填入该数组
    int epollfd_;           // epoll_create()创建成功返回的文件描述符
};

}   // namespace net
    
}   // namespace myserver