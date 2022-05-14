set(net_SRCS
    EventLoop.cc
    InetAddress.cc
    Socket.cc
    SocketOps.cc)

add_library(myserver_net ${net_SRCS})
target_link_libraries(myserver_net myserver_base)

install(TARGETS myserver_net DESTINATION lib)

set(HEADERS
    EventLoop.h
    InetAddress.h
    )
install(FILES ${HEADERS} DESTINATION include/myserver/net)

add_subdirectory(tests)