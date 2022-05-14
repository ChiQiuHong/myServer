/**
* @description: SocketOps.cc
* @author: YQ Huang
* @brief: 封装了socket的一些基本操作
* @date: 2022/05/06 17:31:03
*/

#include "server/net/SocketsOps.h"
#include "server/base/Logging.h"
#include "server/base/Types.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

namespace myserver {

namespace net {

namespace sockets {

/**
 * 这里创建了一系列与socket操作相关的基本函数
 * 如bind() listen() accept() close() read() 等等
 */

#if VALGRIND || defined (NO_ACCEPT4)
void setNonBlockAndCloseOnExec(int sockfd) {
    // 获取文件状态标记
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    // 设置非阻塞属性
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);

    // 获取文件描述符标记
    flags = ::fcntl(sockfd, F_GETFD, 0);
    // 设置文件描述符标记
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);

    (void)ret;
}
#endif

// 设置非阻塞的套接字
int createNonblockingOrDie(sa_family_t family) {
// VALGRIND 内存泄漏探测
#if VALGRIND
    int sockfd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0) {
        LOG_SYSFATAL << "sockets::createNonblockingOrDie";
    }

    setNonBlockAndCloseOnExec(sockfd);
#else
    // SOCK_NONBLOCK 设置非阻塞
    // SOCK_CLOEXEC 用fork创建子进程时在子进程中关闭该socket
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0) {
        LOG_SYSFATAL << "sockets::createNonblockingOrDie";
    }
#endif
    return sockfd;
}

// 请求连接
int connect(int sockfd, const struct sockaddr* addr) {
    return connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

// 向套接字分配网络地址
// 分配失败就会触发FATAL 程序退出
void bindOrDie(int sockfd, const struct sockaddr* addr) {
    int ret = bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if(ret < 0) {
        LOG_SYSFATAL << "sockets::bindOrDie";
    }
}

// 进入等待连接请求状态
// 进入失败就会触发FATAL 程序退出
void listenOrDie(int sockfd) {
    int ret = listen(sockfd, SOMAXCONN);
    if(ret < 0) {
        LOG_SYSFATAL << "sockets::listenOrDie";
    }
}

// 受理客户端连接请求
int accept(int sockfd, struct sockaddr_in6* addr) {
    socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
#if VALGRIND || defined (NO_ACCEPT4)
    int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
    setNonBlockAndCloseOnExec(connfd);
#else
    int connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif  
    if(connfd < 0) {
        int savedErrno = errno;
        LOG_SYSERR << "Socket::accept";
        switch(savedErrno) {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:
            case EPERM:
            case EMFILE:
                errno = savedErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case EOPNOTSUPP:
                LOG_FATAL << "unexpected error of ::accept " << savedErrno;
                break;
            default:
                LOG_FATAL << "unknown error of ::accept " << savedErrno;
                break;
        }
    }
    return connfd;
}

// 读取数据
ssize_t read(int sockfd, void *buf, size_t count) {
    return ::read(sockfd, buf, count);
}

// 读取数据 分散到多个缓冲区
ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt) {
    return ::readv(sockfd, iov, iovcnt);
}

// 写入数据
ssize_t write(int sockfd, const void* buf, size_t count) {
    return ::write(sockfd, buf, count);
}

// 完全断开连接
void close(int sockfd) {
    if(::close(sockfd) < 0) {
        LOG_SYSERR << "sockets::close";
    }
}

// 半关闭 SHUT_WR断开输出流
void shutdownWrite(int sockfd) {
   if(::shutdown(sockfd, SHUT_WR) < 0) {
       LOG_SYSERR << "sockets::shutdwonWrite";
   } 
}

// 生成 IP+Port 字符串 结果保存到 buf 中
void toIpPort(char* buf, size_t size, const struct sockaddr* addr) {
    // IPv6
    if(addr->sa_family == AF_INET6) {
        buf[0] = '[';
        toIp(buf+1, size-1, addr);
        size_t end = strlen(buf);
        const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
        uint16_t port = be16toh(addr6->sin6_port);
        assert(size > end);
        snprintf(buf+end, size-end, "]:%u", port);
        return ;
    }
    // IPv4
    toIp(buf, size, addr);
    size_t end = strlen(buf);
    const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
    uint16_t port = be16toh(addr4->sin_port);
    assert(size > end);
    snprintf(buf+end, size-end, ":%u", port);
}

// 将二进制值转换成点分十进制字符串 结果保存到 buf
void toIp(char* buf, size_t size, const struct sockaddr* addr) {
    // IPv4
    if(addr->sa_family == AF_INET) {
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
        inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
    }
    // IPv6
    else if(addr->sa_family == AF_INET6) {
        assert(size >= INET6_ADDRSTRLEN);
        const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
        inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
    }
}

// 将点分十进制字符串的 IPv4 转换成二进制值 
void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr) {
    addr->sin_family = AF_INET;
    addr->sin_port = htobe16(port);
    if(inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
        LOG_SYSERR << "sockets::fromIpPort";
    }
}

// 将点分十进制字符串的 IPv6 转换成二进制值 
void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr) {
    addr->sin6_family = AF_INET6;
    addr->sin6_port = htobe16(port);
    if(inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) {
        LOG_SYSERR << "sockets::fromIpPort";
    }
}

// 返回Socket错误码
int getSocketError(int sockfd) {
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof(optval));

    // 读取套接字可选项 SO_ERROR 结果保存在 optval
    if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        return errno;
    }
    else {
        return optval;
    }
}

// 将专用IPv4地址转换为通用地址
const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr) {
    return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

// 将专用IPv6地址转换为通用地址
const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr) {
    return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

// 将专用IPv6地址转换为通用地址 非const版本
struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr) {
    return static_cast<struct sockaddr*>(implicit_cast<void*>(addr));
}

// 将通用地址转换为专用IPv4地址
const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr) {
    return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(addr));
}

// 将通用地址转换为专用IPv6地址
const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr) {
    return static_cast<const struct sockaddr_in6*>(implicit_cast<const void*>(addr));
}

// 获取本地套接字的名字，包括它的IP和端口 IPv6
struct sockaddr_in6 getLocalAddr(int sockfd) {
    struct sockaddr_in6 localaddr;
    memZero(&localaddr, sizeof(localaddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(localaddr));
    if(getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0) {
        LOG_SYSERR << "sockets::getLocalAddr";
    }
    return localaddr;
}

// 获取远程套接字的名字，包括它的IP和端口 IPv6
struct sockaddr_in6 getPeerAddr(int sockfd) {
    struct sockaddr_in6 peeraddr;
    memZero(&peeraddr, sizeof(peeraddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(peeraddr));
    if(getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0) {
        LOG_SYSERR << "sockets::getPeerAddr";
    }
    return peeraddr;
}

// 判断是不是连接到本地
bool isSelfConnect(int sockfd) {
    struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
    if(localaddr.sin6_family == AF_INET) {
        const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
        const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
        return laddr4->sin_port == raddr4->sin_port
            && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
    }
    else if(localaddr.sin6_family == AF_INET6) {
        return localaddr.sin6_port == peeraddr.sin6_port
            && memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
    }
    else {
        return false;
    }
}

}   // namespace sockets

}   // namespace net

}   // namespace myserver