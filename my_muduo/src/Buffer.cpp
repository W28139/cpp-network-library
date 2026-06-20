#include "my_muduo/Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>
#include <cstring>

namespace mymuduo
{

const char Buffer::kCRLF[] = "\r\n";

Buffer::Buffer(size_t initialSize)
    : buffer_(kCheapPrepend + initialSize)
    , readerIndex_(kCheapPrepend)
    , writerIndex_(kCheapPrepend)
{
}

size_t Buffer::readableBytes() const
{
    return writerIndex_ - readerIndex_;
}

size_t Buffer::writableBytes() const
{
    return buffer_.size() - writerIndex_;
}

size_t Buffer::prependableBytes() const
{
    return readerIndex_;
}

const char* Buffer::peek() const
{
    return begin() + readerIndex_;
}

void Buffer::retrieve(size_t len)
{
    if (len < readableBytes())
    {
        readerIndex_ += len;
    }
    else
    {
        retrieveAll();
    }
}

void Buffer::retrieveAll()
{
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
}

std::string Buffer::retrieveAllAsString()
{
    return retrieveAsString(readableBytes());
}

std::string Buffer::retrieveAsString(size_t len)
{
    std::string result(peek(), len);
    retrieve(len);
    return result;
}

void Buffer::ensureWritableBytes(size_t len)
{
    if (writableBytes() < len)
    {
        makeSpace(len);
    }
}

void Buffer::append(const char* data, size_t len)
{
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
}

void Buffer::append(const std::string& str)
{
    append(str.data(), str.size());
}

char* Buffer::beginWrite()
{
    return begin() + writerIndex_;
}

const char* Buffer::beginWrite() const
{
    return begin() + writerIndex_;
}

const char* Buffer::findCRLF() const
{
    const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
    return crlf == beginWrite() ? nullptr : crlf;
}

void Buffer::prepend(const void* data, size_t len)
{
    readerIndex_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d + len, begin() + readerIndex_);
}

ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    // 内存中的二级缓冲：通过 readv 散射读取
    // 即使 Buffer 剩余空间很少，也能一次性读完内核缓冲区的数据到栈缓冲中
    char extrabuf[65536];
    struct iovec vec[2];

    const size_t writable = writableBytes();

    // 第一块区域指向 Buffer 本身的可写空间
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writable;

    // 第二块区域指向栈上的 extrabuf
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // 如果 Buffer 空间足够（比如超过 64KB），则不需要使用 extrabuf
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0)
    {
        *savedErrno = errno;
    }
    else if (static_cast<size_t>(n) <= writable)
    {
        // 读到的数据全部进入了 Buffer
        writerIndex_ += n;
    }
    else
    {
        // Buffer 被填满，且有数据溢出到了 extrabuf
        writerIndex_ = buffer_.size();
        // 将溢出的数据追加到 Buffer 末尾，此时 append 会自动处理扩容
        append(extrabuf, n - writable);
    }

    return n;
}

char* Buffer::begin()
{
    return &*buffer_.begin();
}

const char* Buffer::begin() const
{
    return &*buffer_.begin();
}

void Buffer::makeSpace(size_t len)
{
    /**
     * 如果“前面的空闲空间” + “后面的空闲空间” 还是不够 len
     * 或者前面的空闲空间还没达到 kCheapPrepend 的标准，则直接 resize
     */
    if (writableBytes() + prependableBytes() < len + kCheapPrepend)
    {
        buffer_.resize(writerIndex_ + len);
    }
    else
    {
        // 否则不需要申请新内存，将现存的可读数据平移到最前方，解决碎片问题
        size_t readable = readableBytes();
        std::copy(begin() + readerIndex_,
                  begin() + writerIndex_,
                  begin() + kCheapPrepend);
        
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readable;
    }
}

} // namespace mymuduo