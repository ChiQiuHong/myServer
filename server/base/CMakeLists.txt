set(base_SRCS
    # Logging.cc
    LogStream.cc
    )

add_library(myserver_base ${base_SRCS})

add_subdirectory(tests)