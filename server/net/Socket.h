/**
* @description: Socket.h
* @author: YQ Huang
* @brief: socket函数的封装
* @date: 2022/05/06 14:50:19
*/

#pragma once

#include "server/base/noncopyable.h"

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace myserver {

namespace net {

class InetAddress;

/**
 * 通过RAII实现了套接字的管理 该类仅提供接口 具体操作封装在SocketsOps中
 * 析构时关闭sockfd
 * 它是线程安全的
 */ 
class Socket : noncopyable {
public:
    // 构造函数
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    { }

    // 析构函数
    ~Socket();

    // 返回套接字
    int fd() const { return sockfd_; }
    // 获取tcp信息, 成功返回 True
    bool getTcpInfo(struct tcp_info*) const;
    bool getTcpInfoString(char* buf, int len) const;

    // 分配地址
    void bindAddress(const InetAddress& localaddr);
    // 监听
    void listen();
    
    // 连接
    int accept(InetAddress* peeraddr);

    // 半关闭写
    void shutdownWrite();

    // 设置是否使用 Nagel算法 默认不使用
    void setTcpNoDelay(bool on);
    // 设置Time-wait状态下是否重新分配新的套接字 默认重新分配
    void setReuseAddr(bool on);
    // 设置是否将多个socket绑定在同一个监听端口 默认开启
    void setReusePort(bool on);
    // 是否开启TCP保活机制 默认开启
    void setKeepAlive(bool on);

private:
    const int sockfd_;

};

}   // namespace net

}   // namespace myserver