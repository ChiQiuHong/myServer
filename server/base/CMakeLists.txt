set(base_SRCS
    CurrentThread.cc
    FileUtil.cc
    Logging.cc
    LogFile.cc
    LogStream.cc
    Thread.cc
    Timestamp.cc
    )

add_library(myserver_base ${base_SRCS})
target_link_libraries(myserver_base pthread)

install(TARGETS myserver_base DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/muduo/base)

if(MYSERVER_BUILD_EXAMPLES)
  add_subdirectory(tests)
endif()