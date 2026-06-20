#pragma once

#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <string>

#include "my_muduo/EventLoop.h"
#include "my_muduo/Acceptor.h"
#include "my_muduo/ThreadPool.h"
#include "my_muduo/Connection.h"

namespace mymuduo
{


/**
 * @brief TcpServer 类：对外提供服务的主类
 * 整合了 Acceptor, ThreadPool 和 Connection 管理
 */
class TcpServer
{
public:
    using ConnectionPtr = std::shared_ptr<Connection>;
    using ConnectionCallback = std::function<void(const ConnectionPtr&)>;
    using MessageCallback = std::function<void(const ConnectionPtr&, std::string&)>;

    TcpServer(const std::string& ip, uint16_t port, int threadNum = 3);
    ~TcpServer();

    // 启动服务器
    void start();
    
    // 停止服务器
    void stop();

    // --- 设置用户自定义回调 ---
    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
    void setOnMessageCallback(MessageCallback cb) { onMessageCallback_ = std::move(cb); }
    void setSendCompleteCallback(ConnectionCallback cb) { sendCompleteCallback_ = std::move(cb); }
    void setCloseCallback(ConnectionCallback cb) { closeCallback_ = std::move(cb); }

    void enableWorkPool(int threadNum, PoolMode mode = PoolMode::MODE_FIXED);

    template<typename Func>
    void runInWorkPool(Func&& func)
    {
        if(workThreadPool_)
        {
            workThreadPool_->addTask(
                std::forward<Func>(func));
        }
        else
        {
            func();
        }
    }

private:
    // 处理新连接的回调 (供 Acceptor 调用)
    void handleNewConnection(std::unique_ptr<Socket> clientSock);
    
    // 连接关闭的回调 (供 Connection 调用)
    void handleClose(const ConnectionPtr& conn);
    
    // 连接错误的回调
    void handleError(const ConnectionPtr& conn);
    
    // 消息到达的回调
    void handleMessage(const ConnectionPtr& conn, std::string& message);
    
    // 发送完成的回调
    void handleSendComplete(const ConnectionPtr& conn);
    
    // 定时器超时清理
    void removeConnection(int fd);

private:
    // 事件循环相关
    std::unique_ptr<EventLoop> mainLoop_;               // 主循环：负责 Accept
    std::vector<std::unique_ptr<EventLoop>> subLoops_;  // 从循环列表：负责已连接 I/O
    
    ThreadPool ioThreadPool_;                           // IO线程池（固定）
    int threadNum_;                                     // IO线程数量

    std::unique_ptr<ThreadPool> workThreadPool_;        // Work线程池（可选）

    Acceptor acceptor_;                                 // 接收器

    // 连接管理
    std::mutex mutex_;
    std::map<int, ConnectionPtr> connections_;          // 保存所有活动的连接

    // 用户注册的回调
    ConnectionCallback connectionCallback_;
    MessageCallback onMessageCallback_;
    ConnectionCallback sendCompleteCallback_;
    ConnectionCallback closeCallback_;
};
    


} // namespace mymuduo