/**
* @description: EchoServer_unittest.cc
* @author: YQ Huang
* @brief: TcpServer 测试
* @date: 2022/06/26 15:44:37
*/

#include "server/net/TcpServer.h"
#include "server/base/Logging.h"
#include "server/base/Thread.h"
#include "server/net/EventLoop.h"
#include "server/net/InetAddress.h"

#include <utility>
#include <stdio.h>
#include <unistd.h>

using namespace myserver;
using namespace myserver::net;

int numThreads = 0;

class EchoServer {
public:
    EchoServer(EventLoop* loop, const InetAddress& listenAddr)
        : loop_(loop),
          server_(loop, listenAddr, "EchoServer")
    {
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, _1));
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, _1, _2, _3));
    }

    void start() {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr& conn) {
        LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
                  << conn->localAddress().toIpPort() << " is "
                  << (conn->connected() ? "UP" : "DOWN");
        LOG_INFO << conn->getTcpInfoString();

        conn->send("hello\n");
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
        string msg(buf->retrieveAllAsString());
        LOG_TRACE << conn->name() << " recv " << msg.size() << " bytes at " << time.toString();
        if(msg == "exit\n") {
            conn->send("byt\n");
            conn->shutdown();
        }
        if(msg == "quit\n") {
            loop_->quit();
        }
        conn->send(msg);
    }

    EventLoop* loop_;
    TcpServer server_;
};


int main(int argc, char* argv[]) {
    LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
    LOG_INFO << "sizeof TcpConnection = " << sizeof(TcpConnection);
    if(argc > 1) {
        numThreads = atoi(argv[1]);
    }
    bool ipv6 = argc > 2;
    EventLoop loop;
    InetAddress listenAddr(2000, false, ipv6);
    EchoServer server(&loop, listenAddr);

    server.start();
    loop.loop();
}