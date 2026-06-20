#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <cstddef>

namespace mymuduo
{

/**
 * @brief 网络库底层缓冲区类型
 * 
 * 内存布局示意图：
 * +-------------------+------------------+------------------+
 * | prependable bytes |  readable bytes  |  writable bytes  |
 * |                   |     (CONTENT)    |                  |
 * +-------------------+------------------+------------------+
 * |                   |                  |                  |
 * 0      <=      readerIndex    <=   writerIndex    <=    size
 */
class Buffer
{
public:
    // 初始预留 8 字节（用于存放包长度等），初始缓冲区大小 1024 字节
    static constexpr size_t kCheapPrepend = 8;
    static constexpr size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize);

    // --- 基础状态接口 ---

    size_t readableBytes() const;

    size_t writableBytes() const;

    size_t prependableBytes() const;

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const;

    // --- 数据提取接口 (Read) ---

    // 移动读指针，表示消费了 len 长度的数据
    void retrieve(size_t len);

    // 重置读写指针（清空缓冲区）
    void retrieveAll();

    // 将所有可读数据转为字符串返回，并清空缓冲区
    std::string retrieveAllAsString();

    // 将指定长度数据转为字符串返回，并移动读指针
    std::string retrieveAsString(size_t len);

    // --- 数据写入接口 (Write) ---

    // 确保缓冲区有足够的空间写入 len 长度的数据
    void ensureWritableBytes(size_t len);

    // 追加数据
    void append(const char* data, size_t len);

    void append(const std::string& str);

    // 返回可写位置的起始指针
    char* beginWrite();

    const char* beginWrite() const;

    // --- 协议处理辅助 ---

    // 查找 \r\n (CRLF)
    const char* findCRLF() const;

    // 在数据头部前插入数据
    void prepend(const void* data, size_t len);

    // 直接从 Socket 读取数据到缓冲区 (高性能核心)
    ssize_t readFd(int fd, int* savedErrno);

private:
    // 获取底层 vector 起始指针
    char* begin();

    const char* begin() const;

    // 扩容或碎片整理
    void makeSpace(size_t len);

private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

    static const char kCRLF[];
};

} // namespace mymuduo

