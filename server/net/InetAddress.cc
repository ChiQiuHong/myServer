/**
* @description: InetAddresss.cc
* @author: YQ Huang
* @brief: 对IP地址进行封装
* @date: 2022/05/06 15:38:07
*/

#include "server/net/InetAddress.h"
#include "server/base/Logging.h"
#include "server/net/SocketsOps.h"

#include <netdb.h>
#include <netinet/in.h>

// INADDR_ANY use (type)value casting.
#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
#pragma GCC diagnostic error "-Wold-style-cast"

namespace myserver {

namespace net {

static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6),
              "InetAddress is same size as sockaddr_in6");
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");


// 构造函数 根据条件生成ipv4或ipv6地址
InetAddress::InetAddress(uint16_t portArg, bool loopbackOnly, bool ipv6) {
    static_assert(offsetof(InetAddress, addr6_) == 0, "addr6_ offset 0");
    static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");
    if(ipv6) {
        memZero(&addr6_, sizeof addr6_);
        addr6_.sin6_family = AF_INET6;
        // loopback环回地址 只能收到本机上的连接请求 any绑定到全0地址的 是能任意一块网卡的请求
        in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
        addr6_.sin6_addr = ip;
        addr6_.sin6_port = htobe16(portArg);
    }
    else {
        memZero(&addr_, sizeof addr_);
        addr_.sin_family = AF_INET;
        in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
        addr_.sin_addr.s_addr = htobe32(ip);
        addr_.sin_port = htobe16(portArg);
    }
}

// 构造函数 根据给定的IP和端口号生成ipv4或ipv6地址
InetAddress::InetAddress(StringArg ip, uint16_t portArg, bool ipv6) {
    if(ipv6 || strchr(ip.c_str(), ':')) {
        memZero(&addr6_, sizeof addr6_);
        sockets::fromIpPort(ip.c_str(), portArg, &addr6_);
    }
    else {
        memZero(&addr_, sizeof addr_);
        sockets::fromIpPort(ip.c_str(), portArg, &addr_);
    }
}

// 将二进制值 IP 转换成点分十进制字符串 结果保存到 buf
string InetAddress::toIp() const {
    char buf[64] = "";
    sockets::toIp(buf, sizeof buf, getSockAddr());
    return buf;
}

// 将二进制值 IP+Port 转换成点分十进制字符串 结果保存到 buf
string InetAddress::toIpPort() const {
    char buf[64] = "";
    sockets::toIp(buf, sizeof buf, getSockAddr());
    return buf;
}

// 把网络字节序的port转换成主机字节序并返回
uint16_t InetAddress::port() const {
    return be16toh(portNetEndian());
}

// 返回IPv4地址的网络字节序
uint32_t InetAddress::ipv4NetEndian() const {
    assert(family() == AF_INET);
    return addr_.sin_addr.s_addr;
}

// 临时缓冲区 用来存储域名解析的各种信息
static __thread char t_resolveBuffer[64 * 1024];

// 根据域名解析IP地址 解析成功返回true 地址保存在InetAddress指向的地址中
bool InetAddress::resolve(StringArg hostname, InetAddress* out) {
    assert(out != NULL);
    struct hostent hent;
    struct hostent* he = NULL;
    int herrno = 0;
    memZero(&hent, sizeof(hent));

    int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
    if(ret == 0 && he != NULL) {
        assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
        out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
        return true;
    }
    else {
        if(ret) {
            LOG_SYSERR << "InetAddress::resolve";
        }
        return false;
    }
}

// 给IPv6的scopeid赋值
void InetAddress::setScopeId(uint32_t scope_id) {
    if(family() == AF_INET6) {
        addr6_.sin6_scope_id = scope_id;
    }
}

}   // namespace net

}   // namespace myserver