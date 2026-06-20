#include "my_muduo/Buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

// 定义 HTTP 协议常用的换行符，用于解析请求行和请求头
const char Buffer::kCRLF[] = "\r\n";

/**
 * 构造函数
 * @param initialSize 缓冲区的初始大小（不含预留空间）
 * 内存布局：| kCheapPrepend (8字节) | initialSize (默认1024字节) |
 */
Buffer::Buffer(size_t initialSize)
    : buffer_(kCheapPrepend + initialSize), // 实际分配内存
      readerIndex_(kCheapPrepend),         // 初始化读指针，指向数据起始位
      writerIndex_(kCheapPrepend)          // 初始化写指针，此时缓冲区为空
{}

/**
 * 移动读指针（消费数据）
 * @param len 已经处理（读取）的数据长度
 */
void Buffer::retrieve(size_t len)
{
    if (len < readableBytes()) 
	{
        // 如果只读取了一部分，读指针向后移动
        readerIndex_ += len;
    } 
	else 
	{
        // 如果读取了全部数据，直接重置读写指针
        retrieveAll();
    }
}

/**
 * 重置缓冲区
 * 将读写指针恢复到初始位置（kCheapPrepend），逻辑上清空缓冲区
 */
void Buffer::retrieveAll() 
{
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
}

/**
 * 将所有可读数据提取为字符串（常用语业务层获取原始数据）
 */
std::string Buffer::retrieveAllAsString() 
{
    return retrieveAsString(readableBytes());
}

/**
 * 将指定长度的数据提取为字符串
 * @param len 要提取的长度
 */
std::string Buffer::retrieveAsString(size_t len) 
{
    // 构造字符串返回，底层会进行一次内存拷贝
    std::string result(peek(), len);
    // 关键：提取后必须移动读指针，否则数据会一直留在缓冲区
    retrieve(len);
    return result;
}

/**
 * 向缓冲区追加数据
 */
void Buffer::append(const char* data, size_t len) 
{
    // 1. 检查剩余可写空间是否足够，不够则扩容
    ensureWritableBytes(len);
    // 2. 将数据拷贝到写指针指向的位置
    std::copy(data, data + len, beginWrite());
    // 3. 更新写指针
    writerIndex_ += len;
}

void Buffer::append(const std::string& str) 
{
    append(str.data(), str.size());
}

/**
 * 确保有足够的空间可写
 */
void Buffer::ensureWritableBytes(size_t len) 
{
    if (writableBytes() < len) {
        // 进入扩容或碎片整理逻辑
        makeSpace(len);
    }
}

/**
 * 在当前可读数据中查找 \r\n
 * 常用于 HTTP 协议解析请求行（以 \r\n 结尾）
 */
const char* Buffer::findCRLF() const 
{
    const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
    // 如果没找到，search 会返回 beginWrite() 指针
    return crlf == beginWrite() ? nullptr : crlf;
}

/**
 * 内存管理核心：扩容与碎片整理
 * 策略：如果“前面空闲+后面空闲”足够，则挪动数据；否则真正 resize
 */
void Buffer::makeSpace(size_t len) 
{
    /**
     * 情况分析：
     * writableBytes() : 后端空闲空间
     * prependableBytes() : 前端已读数据留下的空闲空间（减去 kCheapPrepend）
     */
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) 
	{
        // 1. 真的没地方了：调用 vector 的 resize 申请更多系统内存
        buffer_.resize(writerIndex_ + len);
    } 
	else 
	{
        // 2. 空间其实够，只是数据太靠后了（碎片化）：
        // 将现存的可读数据 [reader, writer] 移回到起始位置 [kCheapPrepend, ...]
        size_t readable = readableBytes();
        std::copy(begin() + readerIndex_,
                  begin() + writerIndex_,
                  begin() + kCheapPrepend);
        // 更新指针位置
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readable;
    }
}

/**
 * 高性能 I/O 读取函数（核心优化点）
 * 原理：利用 readv 散射读取，结合栈空间防止频繁扩容
 * @param fd 套接字文件描述符
 * @param savedErrno 用于传出系统错误码
 */
ssize_t Buffer::readFd(int fd, int* savedErrno) 
{
    // 栈辅助空间：64KB。
    // 作用：如果内核数据超过了 Buffer 现有的可用空间，溢出的数据会先存在栈上。
    char extrabuf[65536]; 
    
    struct iovec vec[2];
    const size_t writable = writableBytes();
    
    // 第一块区域：指向 Buffer 自带的可写空间
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writable;
    
    // 第二块区域：指向栈上的辅助空间
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // 如果 Buffer 本身空间很大（>64K），则不需要使用栈辅助空间
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    
    /**
     * readv 优势：
     * 1. 原子性读取：一次调用读完内核缓冲区。
     * 2. 自动分流：数据优先填满 vec[0] (Buffer)，多出来的自动进入 vec[1] (extrabuf)。
     */
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0) 
	{
        *savedErrno = errno;
    } 
	else if (static_cast<size_t>(n) <= writable) 
	{
        // 读取的数据量没超过 Buffer 原本的大小，直接更新写指针
        writerIndex_ += n;
    } 
	else 
	{
        // Buffer 被填满了，剩下的数据在 extrabuf 里
        writerIndex_ = buffer_.size();
        // 将栈上的数据 append 到 Buffer 中，此时 append 会触发真正的内存扩容
        append(extrabuf, n - writable);
    }
    return n;
}