/**
* @description: Acceptor.cc
* @author: YQ Huang
* @brief: 用于accept(2)新连接
* @date: 2022/06/11 15:29:45
*/

#include "server/net/Acceptor.h"

#include "server/base/Logging.h"
#include "server/net/EventLoop.h"
#include "server/net/InetAddress.h"
#include "server/net/SocketsOps.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


namespace myserver {

namespace net {

/**
 * 构造函数
 */
Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
    : loop_(loop),
      acceptSocket_(sockets::createNonblockingOrDie(listenAddr.family())),
      acceptChannel_(loop, acceptSocket_.fd()),
      listening_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    assert(idleFd_ >= 0);
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(
        std::bind(&Acceptor::handleRead, this));
}

// 析构函数
Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

// 监听
void Acceptor::listen() {
    loop_->assertInLoopThread();
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

// 调用accept(2)来接受新连接，并回调用户callback
void Acceptor::handleRead() {
    loop_->assertInLoopThread();
    InetAddress peerAddr;

    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0) {
        if(newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        }
        else {
            sockets::close(connfd);
        }
    }
    // 如果connfd小于0，就是说明文件描述符耗尽了，这时候我们关闭预留的dileFd_
    // 那么就会有一个空闲的文件描述符空出来，我们立即去接受新连接，然后立即关闭
    // 重新占用这个空闲的文件描述符。
    else {
        LOG_SYSERR << "in Acceptor::handleRead";
        if(errno == EMFILE) {
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}



}   // namespace net

}   // namespace myserver