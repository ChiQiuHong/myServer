/**
* @description: Acceptor_test.cc
* @author: YQ Huang
* @brief: Acceptor 简单测试
* @date: 2022/06/19 15:25:56
*/


#include "server/net/Acceptor.h"
#include "server/net/EventLoop.h"
#include "server/net/InetAddress.h"

#include <unistd.h>

/**
 * 在1235端口侦听新连接，连接到达后向它发送一个字符串，随机断开连接
 * 
 */

using namespace myserver;


void newConnection(int sockfd, const net::InetAddress& peerAddr) {
    printf("newConnection(): accepted a new connection from %s\n",
           peerAddr.toIpPort().c_str());
    ::write(sockfd, "How are you?\n", 13);
    ::close(sockfd);
}

int main() {
    net::InetAddress listenAddr(1235);
    net::EventLoop loop;

    net::Acceptor acceptor(&loop, listenAddr, true);
    acceptor.setNewConnectionCallback(newConnection);
    acceptor.listen();

    loop.loop();

}
