/**
* @description: TcpServer.h
* @author: YQ Huang
* @brief: TcpServer class 负责管理accept活动的TcpConnection
* @date: 2022/06/26 10:14:35
*/

#pragma once

#include "server/base/Atomic.h"
#include "server/base/Types.h"
#include "server/net/TcpConnection.h"

#include <map>

namespace myserver {

namespace net {

class Acceptor;
class EventLoop;

class TcpServer : noncopyable {
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;
    enum Option {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop,
              const InetAddress& listenAddr,
              const string& nameArg,
              Option option = kNoReusePort);
    ~TcpServer();

    const string& ipPort() const { return ipPort_; }
    const string& name() const { return name_; }
    EventLoop* getLoop() const { return loop_; }

    void start();

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb;}

private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    typedef std::map<string, TcpConnectionPtr> ConnectionMap;

    EventLoop* loop_;
    const string ipPort_;
    const string name_;
    std::unique_ptr<Acceptor> acceptor_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;
    AtomicInt32 started_;
    int nextConnId_;
    ConnectionMap connections_;
};

}   // namespace net

}   // namespace myserver