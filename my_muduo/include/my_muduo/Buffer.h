#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <cstddef>


/**
 * @brief 高性能缓冲区
 * 结构：| prependable bytes |  readable bytes  |  writable bytes  |
 *      |                  |     (CONTENT)    |                  |
 *      0      <=     readerIndex    <=    writerIndex    <=    size
 */
class Buffer {
public:
    static const size_t kInitialSize = 1024;
    static const size_t kCheapPrepend = 8;

    explicit Buffer(size_t initialSize = kInitialSize);

    // --- 基础状态接口 ---
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const { return readerIndex_; }

    // 返回数据起始指针
    const char* peek() const { return begin() + readerIndex_; }

    // --- 数据提取接口 (Read) ---
    void retrieve(size_t len);
    void retrieveAll();
    std::string retrieveAllAsString();
    std::string retrieveAsString(size_t len);

    // --- 数据写入接口 (Write) ---
    void append(const char* data, size_t len);
    void append(const std::string& str);
    void ensureWritableBytes(size_t len);

    // --- 协议处理辅助 ---
    const char* findCRLF() const; // 查找 \r\n，HTTP协议常用
    void prepend(const void* data, size_t len); // 在包头前面填数据

    // 直接从 Socket 读取数据到 Buffer (高性能核心)
    ssize_t readFd(int fd, int* savedErrno);

private:
    char* begin() { return &*buffer_.begin(); }
    const char* begin() const { return &*buffer_.begin(); }

    char* beginWrite() { return begin() + writerIndex_; }
    const char* beginWrite() const { return begin() + writerIndex_; }

    void makeSpace(size_t len);

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

    static const char kCRLF[];
};

