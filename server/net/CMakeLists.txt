set(net_SRCS
    Acceptor.cc
    Buffer.cc
    Channel.cc
    EventLoop.cc
    InetAddress.cc
    Poller.cc
    Socket.cc
    SocketOps.cc
    TcpConnection.cc
    TcpServer.cc
    Timer.cc
    TimerQueue.cc)

add_library(myserver_net ${net_SRCS})
target_link_libraries(myserver_net myserver_base)

install(TARGETS myserver_net DESTINATION lib)

set(HEADERS
    EventLoop.h
    InetAddress.h
    )
install(FILES ${HEADERS} DESTINATION include/myserver/net)

add_subdirectory(tests)