/**
* @description: TcpConnection.cc
* @author: YQ Huang
* @brief: 对每个TCP连接的封装管理
* @date: 2022/06/19 16:07:07
*/

#include "server/net/TcpConnection.h"

#include "server/base/Logging.h"
#include "server/base/WeakCallback.h"
#include "server/net/Channel.h"
#include "server/net/EventLoop.h"
#include "server/net/Socket.h"
#include "server/net/SocketsOps.h"

#include <errno.h>

namespace myserver {

namespace net {

void defaultConnectionCallback(const TcpConnectionPtr& conn) {
    LOG_TRACE << conn->localAddress().toIpPort() << " -> "
              << conn->peerAddress().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr&,
                            Buffer* buf,
                            Timestamp)
{
    buf->retrieveAll();
}

// TcpConnection 没有发起连接的功能，其构造函数的参数是
// 已经建立好连接的socket fd 因此其初始状态是kConnecting
TcpConnection::TcpConnection(EventLoop* loop,
                             const string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
    : loop_(CHECK_NOTNULL(loop)),
      name_(nameArg),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64*1024*1024)
{
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));
    LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
              << " fd=" << sockfd;
    socket_->setKeepAlive(true);
}

// 析构函数中会 close(fd) (在 Socket的析构函数中发生)
TcpConnection::~TcpConnection() {
    LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
              << " fd=" << channel_->fd()
              << " state=" << stateToString();
    assert(state_ == kDisconnected);
}


bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const {
    return socket_->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const {
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof buf);
    return buf;
}

/**
 * @brief 发送数据
 * 实际调用sendInLoop(), 
 */

void TcpConnection::send(const void* data, int len) {
    send(StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::send(const StringPiece& message) {
    if(state_ == kConnected) {
        if(loop_->isInLoopThread()) {
            sendInLoop(message);
        }
        else {
            void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                std::bind(fp, this, message.as_string()));
        }
    }
}

void TcpConnection::send(Buffer* buf) {
    if(state_ == kConnected) {
        if(loop_->isInLoopThread()) {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else {
            void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                std::bind(fp, this, buf->retrieveAllAsString()));
        }
    }
}

void TcpConnection::shutdown() {
    if(state_ == kConnected) {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::forceClose() {
    if(state_ == kConnected || state_ == kDisconnecting) {
        setState(kDisconnecting);
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseWithDelay(double seconds) {
    if(state_ == kConnected || state_ == kDisconnecting) {
        setState(kDisconnecting);
        loop_->runAfter(
            seconds,
            makeWeakCallback(shared_from_this(), &TcpConnection::forceClose));
    }
}

void TcpConnection::setTcpNoDelay(bool on) {
    socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead() {
    loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::stopRead() {
    loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

// TcpServer创建TcpConnection后，注册事件处理回调函数，之后在IO循环中执行
// connectEstablished()函数，开始关注fd的可读事件、回调客户连接成功的函数
void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

// 关闭连接 当TcpServer 从map中移除TcpConnection时调用
void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    if(state_ == kConnected) {
        // 和handleClose() 中重复 是为了处理不经由handleClose() 而是
        // 直接调用connectDestroyed()的情况
        setState(kDisconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }

    channel_->remove(); // 从Poller中移除channel
}

// 当有可读事件发生，执行handleRead()回调。尝试从socketfd中读取数据保存到Buffer中
void TcpConnection::handleRead(Timestamp receiveTime) {
    loop_->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    // 若读取长度大于0，将接收的数据通过messageCallback_传递到上层应用(这里是TcpServer)
    if(n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    // 如果长度等于0，说明对端客户端关闭了连接，调用handleClose()进行关闭处理
    else if (n == 0) {
        handleClose();
    }
    // 如果小于0，调用handleError()进行错误处理
    else {
        errno = savedErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
}

// 内核中为sockfd分配的发送缓冲区未满时，sockfd将一直处于可写的状态，由于
// 采用LT水平触发，需要在发送数据的时候才关注可写事件，否则会造成busy loop
void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if(channel_->isWriting()) { // 当前sockfd可写
        ssize_t n = sockets::write(channel_->fd(),
                                   outputBuffer_.peek(),
                                   outputBuffer_.readableBytes());
        if(n > 0) {
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0) {    // 发送完毕
                channel_->disableWriting(); // 不再关注fd的可写事件，避免busy loop
                if(writeCompleteCallback_) {
                    // 通知用户，发送完毕
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                // 如果当前状态是正在关闭连接，主动发送关闭
                if(state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        }
        else {
            LOG_SYSERR << "TcpConnection::handleWrite";
        }
    }
    else {
        LOG_TRACE << "Connection fd = " << channel_->fd()
                  << " is down, no more writing";
    }
}

// 关闭事件处理
void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    channel_->disableAll(); // channel不再关注任何事情

    TcpConnectionPtr guardThis(shared_from_this()); //必须使用智能指针
    connectionCallback_(guardThis); // 回调用户的连接处理回调函数
    //必须最后调用，回调TcpServer的函数，TcpConnection的生命期由TcpServer控制
    closeCallback_(guardThis);
}

void TcpConnection::handleError() {
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}

void TcpConnection::sendInLoop(const StringPiece& message) {
    sendInLoop(message.data(), message.size());
}

// sendInLoop() 具体负责发送数据
void TcpConnection::sendInLoop(const void* data, size_t len) {
    loop_->assertInLoopThread();
    ssize_t nwrote = 0; // 记录写了多少字节
    size_t remaining = len;
    bool faultError = false;
    if(state_ == kDisconnected) {
        LOG_WARN << "disconnected, give up writing";
        return ;
    }
    // 如果当前channel没有写事件发生，并且发送buffer无待发送数据，那么直接发送
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = sockets::write(channel_->fd(), data, len);
        if(nwrote >= 0) {
            remaining = len - nwrote;
            // 如果一次发送完毕，就调用发送完成回调函数
            if(remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else {  // 一旦发生错误，关闭连接
            nwrote = 0;
            if(errno != EWOULDBLOCK) {
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if(errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }

    assert(remaining <= len);
    // 如果只发送了部分数据，则把剩余的数据放入outputBuffer_，并开始关注可写事件
    if(!faultError && remaining > 0) {
        size_t oldLen = outputBuffer_.readableBytes();
        // 如果输出缓冲区的数据已经超过超高水位标记，那么调用highWaterMarkCallback_
        if(oldLen + remaining >= highWaterMark_
           && oldLen < highWaterMark_
           && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        // 把数据添加到输出缓冲区中
        outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
        // 监听channel的可写事件，因为还有数据未发完
        if(!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if(!channel_->isWriting()) {
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceCloseInLoop() {
    loop_->assertInLoopThread();
    if(state_ == kConnected || state_ == kDisconnecting) {
        handleClose();
    }
}

const char* TcpConnection::stateToString() const {
    switch(state_) {
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unkown state";
    }
}

void TcpConnection::startReadInLoop() {
    loop_->assertInLoopThread();
    if(!reading_ || !channel_->isReading()) {
        channel_->enableReading();
        reading_ = true;
    }
}

void TcpConnection::stopReadInLoop() {
    loop_->assertInLoopThread();
    if(reading_ || channel_->isReading()) {
        channel_->disableReading();
        reading_ = false;
    }
}


}   // namespace net
    
}   // namespace myserver