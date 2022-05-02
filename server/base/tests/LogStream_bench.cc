/**
* @description: LogStream_bench.cc
* @author: YQ Huang
* @brief: LogStream性能测试
* @date: 2022/05/01 20:31:59
*/

#include "server/base/LogStream.h"
#include "server/base/Timestamp.h"

#include <stdio.h>
#include <sstream>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace myserver;

const size_t N = 1000000;

#pragma GCC diagnostic ignored "-Wold-style-cast"

// printf
template<typename T>
void benchPrintf(const char* fmt) {
    char buf[32];
    Timestamp start(Timestamp::now());
    for(size_t i = 0; i < N; ++i) {
        snprintf(buf, sizeof buf, fmt, (T)(i));
    }
    Timestamp end(Timestamp::now());
    printf("benchPrintf %f\n", timeDifference(end, start));
}

// StringStream
template<typename T>
void benchStringStream() {
    std::ostringstream os;
    Timestamp start(Timestamp::now());
    for(size_t i = 0; i < N; ++i) {
        os << (T)(i);
        os.seekp(0, std::ios_base::beg);
    }
    Timestamp end(Timestamp::now());
    printf("benchStringStream %f\n", timeDifference(end, start));
}

// LogStream
template<typename T>
void benchLogStream() {
    myserver::LogStream os;
    Timestamp start(Timestamp::now());
    for(size_t i = 0; i < N; ++i) {
        os << (T)(i);
        os.resetBuffer();
    }
    Timestamp end(Timestamp::now());
    printf("benchLogStream %f\n", timeDifference(end, start));
}

int main() {
    benchPrintf<int>("%d");

    puts("int");
    benchPrintf<int>("%d");
    benchStringStream<int>();
    benchLogStream<int>();

    puts("double");
    benchPrintf<double>("%.12g");
    benchStringStream<double>();
    benchLogStream<double>();

    puts("int64_t");
    benchPrintf<int64_t>("%" PRId64);
    benchStringStream<int64_t>();
    benchLogStream<int64_t>();

    puts("void*");
    benchPrintf<void*>("%p");
    benchStringStream<void*>();
    benchLogStream<void*>();
}