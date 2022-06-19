/**
* @description: TcpConnection.h
* @author: YQ Huang
* @brief: 对每个TCP连接进行的封装管理
* @date: 2022/06/19 16:07:13
*/

#pragma once

#include "server/base/noncopyable.h"
#include "server/base/StringPiece.h"
#include "server/base/Types.h"
#include "server/net/Callbacks.h"
#include "server/net/InetAddress.h"

#include <memory>

#include <boost/any.hpp>

struct tcp_info;

namespace myserver {

namespace net {

class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,
                  const string& name,
                  int sockfd,
                  const InetAddress& localAddr,
                  const InetAddress& peerAddr);
    ~TcpConnection();

private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    void handleRead(Timestamp receiveTime);

    EventLoop* loop_;
    const string name_;
    StateE state_;
    bool reading_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;
    

};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

}   // namespace net
    
}   // namespace myserver