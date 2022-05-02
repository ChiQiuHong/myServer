set(base_SRCS
    FileUtil.cc
    Logging.cc
    LogFile.cc
    LogStream.cc
    Timestamp.cc
    )

add_library(myserver_base ${base_SRCS})

add_subdirectory(tests)