/**
* @description: Socket.cc
* @author: YQ Huang
* @brief: socket函数的封装
* @date: 2022/05/06 14:51:20
*/

#include "server/net/Socket.h"

#include "server/base/Logging.h"
#include "server/net/InetAddress.h"
#include "server/net/SocketsOps.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>

namespace myserver {

namespace net {

// 析构函数
Socket::~Socket() {
    sockets::close(sockfd_);
}

// 获取tcp信息, 成功返回 True
bool Socket::getTcpInfo(struct tcp_info* tcpi) const {
    socklen_t len = sizeof(*tcpi);
    memZero(tcpi, len);
    return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::getTcpInfoString(char* buf, int len) const {
    struct tcp_info tcpi;
    bool ok = getTcpInfo(&tcpi);
    if(ok) {
        snprintf(buf, len, "unrecovered=%u "
                 "rto=%u ato=%u snd_mss=%s rcv_mss=%u "
                 "lost=%u retrans=%u rtt=%u rttvar=%u "
                 "sshthresh=%u cwnd=%u total_retrans=%u",
                 tcpi.tcpi_retransmits,     // 重传数 表示当前待重传的包数
                 tcpi.tcpi_rto,             // 重传超时时间 与rtt有关系
                 tcpi.tcpi_ato,             // 用来延时确认的估值
                 tcpi.tcpi_snd_mss,         // 本端的MSS
                 tcpi.tcpi_rcv_mss,         // 对端的MSS
                 tcpi.tcpi_lost,            // 本端在发送出去丢失的报文数 重传完成后清零
                 tcpi.tcpi_retrans,         // 重传且未确认的数据段数
                 tcpi.tcpi_rtt,             // 往返时延
                 tcpi.tcpi_rttvar,          // RTT的平均偏差 该值越大 RTT抖动越大
                 tcpi.tcpi_snd_ssthresh,    // 拥塞控制慢开始阈值
                 tcpi.tcpi_snd_cwnd,        // 拥塞控制窗口
                 tcpi.tcpi_total_retrans);  // 统计总重传的包数
    }
    return ok;
}

// 分配地址
void Socket::bindAddress(const InetAddress& addr) {
    sockets::bindOrDie(sockfd_, addr.getSockAddr());
}

// 监听
void Socket::listen() {
    sockets::listenOrDie(sockfd_);
}

// 连接
int Socket::accept(InetAddress* peeraddr) {
    struct sockaddr_in6 addr;
    memZero(&addr, sizeof(addr));
    int connfd = sockets::accept(sockfd_, &addr);
    if(connfd >= 0) {
        peeraddr->setSockAddrInet6(addr);
    }
    return connfd;
}

// 半关闭写
void Socket::shutdownWrite() {
    sockets::shutdownWrite(sockfd_);
}

// 设置是否使用 Nagel算法 默认不使用
void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
                 &optval, static_cast<socklen_t>(sizeof(optval)));
}

// 设置Time-wait状态下是否重新分配新的套接字 默认重新分配
void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
                 &optval, static_cast<socklen_t>(sizeof(optval)));
}

// 设置是否将多个socket绑定在同一个监听端口 默认开启
void Socket::setReusePort(bool on) {
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
                           &optval, static_cast<socklen_t>(sizeof(optval)));
    if(ret < 0 && on) {
        LOG_SYSERR << "SO_REUSEPORT failed.";
    }
#else
    if(on) {
        LOG_ERROR << "SO_REUSEPORT is not supported.";
    }
#endif
}

// 是否开启TCP保活机制 默认开启
void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, 
                 &optval, static_cast<socklen_t>(sizeof(optval)));
}

}   // namespace net

}   // namespace myserver