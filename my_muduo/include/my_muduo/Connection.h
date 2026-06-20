#pragma once

#include <functional>
#include <memory>
#include <atomic>
#include <string>

#include "my_muduo/Buffer.h"
#include "my_muduo/Timestamp.h"

namespace mymuduo
{

class EventLoop;
class Socket;
class Channel;

/**
 * @brief TCP 连接类
 * 封装了一个已建立的客户端连接、对应的 Socket 和 Channel，
 * 以及该连接特有的输入输出缓冲区。
 */
class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using ConnectionPtr = std::shared_ptr<Connection>;
    using MessageCallback = std::function<void(const ConnectionPtr&, std::string&)>;
    using Callback = std::function<void(const ConnectionPtr&)>;

    Connection(EventLoop* loop, std::unique_ptr<Socket> clientSock);
    ~Connection();

    // 获取基本信息
    int fd() const;
    std::string ip() const;
    uint16_t port() const;

    // 发送数据
    void send(const std::string& data);

    // 设置各类回调
    void setOnMessageCallback(MessageCallback cb) { onMessageCallback_ = std::move(cb); }
    void setSendCompleteCallback(Callback cb) { sendCompleteCallback_ = std::move(cb); }
    void setCloseCallback(Callback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(Callback cb) { errorCallback_ = std::move(cb); }

    // 超时判断
    bool isTimeout(time_t now, int seconds) const;

    // 状态管理
    bool connected() const { return !disconnected_; }

private:
    // Channel 的事件分发回调
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    // 在所属的 Loop 线程中发送数据
    void sendInLoop(const std::string& data);

private:
    EventLoop* loop_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    Buffer inputBuffer_;  // 接收缓冲区
    Buffer outputBuffer_; // 发送缓冲区

    std::atomic_bool disconnected_; // 连接断开标志

    // 业务层回调
    MessageCallback onMessageCallback_;
    Callback sendCompleteCallback_;
    Callback closeCallback_;
    Callback errorCallback_;

    Timestamp lastActiveTime_; // 最后活跃时间戳
};

} // namespace mymuduo