# add_executable(logging_test Logging_test.cc)
# target_link_libraries(logging_test myserver_base)

add_executable(logstream_bench LogStream_bench.cc)
target_link_libraries(logstream_bench myserver_base)