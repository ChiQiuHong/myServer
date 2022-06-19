/**
* @description: Acceptor.h
* @author: YQ Huang
* @brief: 用于accept(2)新连接
* @date: 2022/06/11 15:30:15
*/

#pragma once

#include "server/net/Channel.h"
#include "server/net/Socket.h"

#include <functional>

namespace myserver {

namespace net {

class EventLoop;
class InetAddress;

/**
 * Acceptor class 用于accept(2)新TCP连接，并通过回调通知使用者。
 * 它是内部class，供TcpServer使用，生命期由后者控制
 */
class Acceptor : noncopyable {
public:
    typedef std::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }

    void listen();
    bool listening() const { return listening_; }

private:
    void handleRead();  // 调用accept(2)来接受新连接，并回调用户callback

    EventLoop* loop_;
    Socket acceptSocket_;       // listening socket， 即server socket
    Channel acceptChannel_;     // 用于观察此socket上的readable事件，并回调Acceptor::handleRead()
    NewConnectionCallback newConnectionCallback_;
    bool listening_;
    int idleFd_;    // idle为空闲的意思，用作占位的空闲描述符，见muduo7.7节
};


}   // namespace net

}   // namespace myserver