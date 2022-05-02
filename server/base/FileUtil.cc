/**
* @description: FileUtil.cc
* @author: YQ Huang
* @brief: 文件操作类
* @date: 2022/05/02 17:40:46
*/

#include "server/base/FileUtil.h"
#include "server/base/Logging.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace myserver {

namespace FileUtil {

// ReadSmallFil构造函数
ReadSmallFile::ReadSmallFile(StringArg filename)
    : fd_(open(filename.c_str(), O_RDONLY | O_CLOEXEC)),
      err_(0)
{
    buf_[0] = '\0';
    if(fd_ < 0) {
        err_ = errno;
    }
} 

// ReadSmallFil析构函数
ReadSmallFile::~ReadSmallFile() {
    if(fd_ >= 0) {
        close(fd_);
    }
}

// 获取文件信息，并把文件内容读入到content中 返回错误码errno
template<typename String>
int ReadSmallFile::readToString(int maxSize,
                                String* content,
                                int64_t* fileSize,
                                int64_t* modifyTime,
                                int64_t* createTime)
{   
    // 编译期间断言 long int 是否占 8位
    static_assert(sizeof(off_t) == 8, "_FILE_OFFSET_BITS = 64");
    assert(content != NULL);
    int err = err_;
    if(fd_ >= 0) {
        // 清空content，准备读入
        content->clear();
    }
    if(fileSize) {
        struct stat statbuf;    // fstat函数通过文件描述符来获取文件的属性，结果保存到statbuf中
        if(fstat(fd_, &statbuf) == 0) {
            // 是否是一个常规文件
            if(S_ISREG(statbuf.st_mode)) {
                // 为字符串分配足够的空间
                *fileSize = statbuf.st_size;
                content->reserve(static_cast<int>(std::min(implicit_cast<int64_t>(maxSize), *fileSize)));
            }
            // 是否是一个目录
            else if(S_ISDIR(statbuf.st_mode)) {
                err = EISDIR;
            }
            if(modifyTime) {
                *modifyTime = statbuf.st_mtime; // 修改时间
            }
            if(createTime) {
                *createTime = statbuf.st_ctime; // 创建时间
            }
        }
        else {
            err = errno;
        }
    }

    // 循环 读取字符进入content，其中buf_用于协助读取字符，防止溢出
    while(content->size() < implicit_cast<size_t>(maxSize)) {
        // 一次读入buf_个字符
        size_t toRead = std::min(implicit_cast<size_t>(maxSize) - content->size(), sizeof(buf_));
        ssize_t n = read(fd_, buf_, toRead);
        if(n > 0) {
            content->append(buf_, n);
        }
        else {
            if(n < 0) {
                err = errno;
            }
            break;
        }
    }
    return err;
}
    
// 读取 kBufferSize 个字符到缓冲区 并给size赋值为文件的大小
// 返回 错误码errno
int ReadSmallFile::readToBuffer(int* size) {
    int err = err_;
    if(fd_ >= 0) {
        ssize_t n = pread(fd_, buf_, sizeof(buf_) - 1, 0);
        if(n >= 0) {
            if(size) {
                *size = static_cast<int>(n);
            }
            buf_[n] = '\0';
        }
        else {
            err = errno;
        }
    }
    return err;
}

// 实例化
template int readFile(StringArg filename,
                      int maxSize,
                      string* content,
                      int64_t* , int64_t*, int64_t*);

template int ReadSmallFile::readToString(
    int maxSize,
    string* content,
    int64_t* , int64_t*, int64_t*);   


// AppendFile构造函数     
AppendFile::AppendFile(StringArg filename)
    : fp_(fopen(filename.c_str(), "ae")), //a：追加 e：O_CLOEXEC 多线程
      writtenBytes_(0)
{
    assert(fp_);
    // 设置文件流的缓冲区
    setbuffer(fp_, buffer_, sizeof buffer_);
}

// AppendFile的析构函数
AppendFile::~AppendFile() {
    fclose(fp_);
}

// 将logline写到缓冲区
void AppendFile::append(const char* logline, size_t len) {
    size_t written = 0; // 记录已写字节数
    
    while(written != len) {
        size_t remain = len - written;
        // 调用私有函数write 具体实现是fwrite_unlocked
        size_t n = AppendFile::write(logline + written, remain);
        if(n != remain) {
            int err = ferror(fp_);
            if(err) {
                fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
                break;
            }
        }
        written += n;
    }

    writtenBytes_ += written;
}

// 将缓冲区的信息flush到输出
void AppendFile::flush() {
    fflush(fp_);
}

size_t AppendFile::write(const char* logline, size_t len) {
    return fwrite_unlocked(logline, 1, len, fp_);
}

}   // namespace FileUtil

}   // namespace myserver