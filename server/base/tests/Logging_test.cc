/**
* @description: Logging_test.cc
* @author: YQ Huang
* @brief: Logging测试文件
* @date: 2022/05/01 09:41:15
*/

#include "server/base/Logging.h"
#include "server/base/LogFile.h"
#include "server/base/ThreadPool.h"

#include <stdio.h>
#include <unistd.h>

int g_total;
FILE* g_file;
std::unique_ptr<myserver::LogFile> g_logFile;

void dummyOutput(const char* msg, int len) {
    g_total += len;
    if(g_file) {
        fwrite(msg, 1, len, g_file);
    }
    else if (g_logFile) {
        g_logFile->append(msg, len);
    }
}

void bench(const char* type) {
    myserver::Logger::setOutput(dummyOutput);
    myserver::Timestamp start(myserver::Timestamp::now());
    g_total = 0;

    int n = 1000*1000;
    const bool kLongLog = false;
    std::string empty = " ";
    std::string longStr(3000, 'X');
    longStr += " ";
    for(int i = 0; i < n; ++i) {
        LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz"
                 << (kLongLog ? longStr : empty)
                 << i;
    }
    myserver::Timestamp end(myserver::Timestamp::now());
    double seconds = myserver::timeDifference(end, start);
    printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s\n",
           type, seconds, g_total, n / seconds, g_total / seconds / (1024*1024));
}

void logInThread() {
    LOG_INFO << "logInThread";
    usleep(1000);
}

int main() {

    getppid();

    myserver::ThreadPool pool("pool");
    pool.start(5);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);

    LOG_TRACE << "trace";
    LOG_DEBUG << "debug";
    LOG_INFO << "Hello";
    LOG_WARN << "World";
    LOG_ERROR << "Error";
    LOG_INFO << sizeof(myserver::Logger);
    LOG_INFO << sizeof(myserver::LogStream);
    LOG_INFO << sizeof(myserver::Fmt);
    LOG_INFO << sizeof(myserver::LogStream::Buffer);

    sleep(1);
    bench("nop");

    char buffer[64 * 1024];
    
    g_file = fopen("/dev/null", "w");
    setbuffer(g_file, buffer, sizeof(buffer));
    bench("dev/null");
    fclose(g_file);

    g_file = fopen("/home/workspace/myServer/build/log", "w");
    setbuffer(g_file, buffer, sizeof(buffer));
    bench("myServer/build");
    fclose(g_file);

    g_file = NULL;
    g_logFile.reset(new myserver::LogFile("test_log_st", 500*1000*1000, false));
    bench("test_log_st");

    g_logFile.reset(new myserver::LogFile("test_log_mt", 500*1000*1000, true));
    bench("test_log_st");
}