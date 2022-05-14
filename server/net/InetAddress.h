/**
* @description: InetAddress.h
* @author: YQ Huang
* @brief: 对IP地址进行封装
* @date: 2022/05/06 15:38:11
*/

#pragma once

#include "server/base/copyable.h"
#include "server/base/StringPiece.h"

#include <netinet/in.h>

namespace myserver {

namespace net {

namespace sockets {

const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);

}   // namespace sockets

/**
 * 对socket地址封装
 */
class InetAddress : public copyable {
public:
    // 构造函数
    explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);

    InetAddress(StringArg ip, uint16_t port, bool ipv6 = false);

    explicit InetAddress(const struct sockaddr_in& addr)
        : addr_(addr)
    { }

    explicit InetAddress(const struct sockaddr_in6& addr)
        : addr6_(addr)
    { }

    sa_family_t family() const { return addr_.sin_family; } // 返回地址族类型
    string toIp() const;        // 
    string toIpPort() const;
    uint16_t port() const;      // 把网络字节序的port转换成主机字节序并返回

    // 返回IPv6的socket地址结构体
    const struct sockaddr* getSockAddr() const { return sockets::sockaddr_cast(&addr6_); }
    // 设置IPv6的地址结构体
    void setSockAddrInet6(const struct sockaddr_in6& addr6) { addr6_ = addr6; }

    // 返回IPv4地址的网络字节序
    uint32_t ipv4NetEndian() const;
    // 返回网络字节序的端口号
    uint16_t portNetEndian() const { return addr_.sin_port; }

    // 根据域名解析IP地址 解析成功返回true 地址保存在InetAddress指向的地址中
    static bool resolve(StringArg hostname, InetAddress* result);
    
    // 给IPv6的scopeid赋值
    void setScopeId(uint32_t scope_id);

private:
    // 联合体类型的addr 保存IPv4或IPv6地址
    union {
        // sockadd_in 专用socket地址
        struct sockaddr_in addr_;
        struct sockaddr_in6 addr6_;
    };
};

}   // namespace net

}   // namespace myserver