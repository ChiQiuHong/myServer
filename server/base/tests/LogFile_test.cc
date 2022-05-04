/**
* @description: LogFile_test.cc
* @author: YQ Huang
* @brief: LogFile类测试函数
* @date: 2022/05/02 19:49:12
*/

#include "server/base/LogFile.h"
#include "server/base/Logging.h"

#include <unistd.h>

std::unique_ptr<myserver::LogFile> g_logFile;

void outputFunc(const char* msg, int len) {
    g_logFile->append(msg, len);
}

void flushFunc() {
    g_logFile->flush();
}

int main(int argc, char* argv[]) {
    char name[256] = { '\0' };
    strncpy(name, argv[0], sizeof name - 1);
    g_logFile.reset(new myserver::LogFile(basename(name), 200*1000));
    myserver::Logger::setOutput(outputFunc);
    myserver::Logger::setFlush(flushFunc);

    std::string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    for(int i = 0; i < 10000; ++i) {
        LOG_INFO << line << i;

        usleep(1000);
    }
}