/**
* @description: Buffer.cc
* @author: YQ Huang
* @brief: 输入输出缓冲区
* @date: 2022/06/19 17:07:02
*/

#include "server/net/Buffer.h"

#include "server/net/SocketsOps.h"

#include <errno.h>
#include <sys/uio.h>

namespace myserver {

namespace net {

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

ssize_t Buffer::readFd(int fd, int* savedErrno) {
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writerIndex_;
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = sockets::readv(fd, vec, iovcnt);
    if(n < 0) {
        *savedErrno = errno;
    }
    else if (implicit_cast<size_t>(n) <= writable) {
        writerIndex_ += n;
    }
    else {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }

    return n;
}

}   // namespace net

}   // namespace myserver