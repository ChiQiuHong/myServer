/**
* @description: Logging_test.cc
* @author: YQ Huang
* @brief: Logging测试文件
* @date: 2022/05/01 09:41:15
*/

#include "server/base/Logging.h"

#include <iostream>

int main() {
    std::cout << "Hello World" << std::endl;
    std::cout << myserver::Logger(__FILE__, __LINE__).stream();
}