/**
* @description: FileUtil.h
* @author: YQ Huang
* @brief: 文件操作类
* @date: 2022/05/02 17:40:38
*/

#pragma once

#include "server/base/noncopyable.h"
#include "server/base/StringPiece.h"
#include <sys/types.h> // for off_t


namespace myserver {

namespace FileUtil {

/**
 * 读取小文件 < 64KB
 */
class ReadSmallFile : noncopyable {
public:
    // StringArg 就是 const char*
    ReadSmallFile(StringArg filename);
    ~ReadSmallFile();

    // 读取字符串 返回错误码errno
    template<typename String>
    int readToString(int maxSize,
                     String* content,
                     int64_t* fileSize,
                     int64_t* modifyTime,
                     int64_t* createTime);
    
    // 读取 kBufferSize 个字符到缓冲区
    // 返回 错误码errno
    int readToBuffer(int* size);

    // 返回缓冲区
    const char* buffer() const { return buf_; }

    static const int kBufferSize = 64*1024; // 缓冲区大小为 64KB

private:
    int fd_;                    // 文件操作符
    int err_;                   // 错误码
    char buf_[kBufferSize];    // 缓冲区
};

// 读取文件的模板类，返回错误码 内部调用ReadSmallFile.readToString
template<typename String>
int readFile(StringArg filename,
             int maxSize,
             String* content,
             int64_t* fileSize = NULL,
             int64_t* modifyTime = NULL,
             int64_t* createTime = NULL)
{
    ReadSmallFile file(filename);
    return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
}

/**
 * AppendFile不是线程安全的，由调用它的函数保证线程安全
 */
class AppendFile : noncopyable {
public:
    explicit AppendFile(StringArg filename);
    
    ~AppendFile();

    // 将logline写到缓冲区
    void append(const char* logline, size_t len);
    // 将缓冲区的信息flush到输出
    void flush();
    // 已写入的字节数
    off_t writtenBytes() const { return writtenBytes_;}

private:
    // 写
    size_t write(const char* logline, size_t len);

    FILE* fp_;  // 打开的文件指针
    char buffer_[64*1024];  // 用户态缓冲区 大小为64KB 减少文件IO的次数
    off_t writtenBytes_;    // 已写入字节数
};

}   // namespace FileUtil

}   // namespace myserver