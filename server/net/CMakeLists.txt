set(net_SRCS
    InetAddress.cc
    Socket.cc
    SocketOps.cc)

add_library(myserver_net ${net_SRCS})
target_link_libraries(myserver_net myserver_base)

install(TARGETS myserver_net DESTINATION lib)