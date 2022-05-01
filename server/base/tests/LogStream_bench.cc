/**
* @description: LogStream_bench.cc
* @author: YQ Huang
* @brief: LogStream性能测试
* @date: 2022/05/01 20:31:59
*/

#include "server/base/LogStream.h"

#include <stdio.h>
#include <sys/time.h>

const size_t N = 1000000;

template<typename T>
void benchLogStream() {
    myserver::LogStream os;
    struct timeval start, end;
    double timeuse;
    gettimeofday(&start, NULL);
    for(size_t i = 0; i < N; ++i) {
        os << (T)(i);
        os.resetBuffer();
    }
    gettimeofday(&end, NULL);
    timeuse = (double)(end.tv_usec - start.tv_usec) / 1000000.0;
    printf("benchLogStream %lf\n", timeuse);
}

int main() {
    benchLogStream<int>();
}