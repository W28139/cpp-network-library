#include "my_muduo/TcpServer.h"
#include "my_muduo/Connection.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/syscall.h>

using namespace mymuduo;

// 获取当前线程ID
static pid_t getTid()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

class EchoServer
{
public:
    EchoServer(const std::string& ip,
               uint16_t port,
               int ioThreads)
        : server_(ip, port, ioThreads)
    {
        // 开启业务线程池
        server_.enableWorkPool(
            8,
            PoolMode::MODE_CACHED);

        // 注册回调
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection,
                      this,
                      std::placeholders::_1));

        server_.setOnMessageCallback(
            std::bind(&EchoServer::onMessage,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2));

        server_.setCloseCallback(
            std::bind(&EchoServer::onClose,
                      this,
                      std::placeholders::_1));
    }

    void start()
    {
        server_.start();
    }

private:
    void onConnection(const TcpServer::ConnectionPtr& conn)
    {
        std::cout
            << "[NEW CONNECTION] "
            << conn->ip() << ":"
            << conn->port()
            << " tid=" << getTid()
            << std::endl;
    }

    void onClose(const TcpServer::ConnectionPtr& conn)
    {
        std::cout
            << "[CONNECTION CLOSED] "
            << conn->ip() << ":"
            << conn->port()
            << " tid=" << getTid()
            << std::endl;
    }

    void onMessage(const TcpServer::ConnectionPtr& conn,
                   std::string& msg)
    {
        std::cout
            << "[MESSAGE] recv: "
            << msg
            << " worker tid="
            << getTid()
            << std::endl;

        // 模拟耗时业务
        std::this_thread::sleep_for(
            std::chrono::seconds(3));

        std::string response =
            "Echo from worker tid="
            + std::to_string(getTid())
            + " : " + msg;

        // 注意：
        // 当前在Work线程中
        // send()会自动切换到所属IO线程执行
        conn->send(response);
    }

private:
    TcpServer server_;
};


int main()
{
    EchoServer server(
        "0.0.0.0",
        8080,
        4); // 4个IO线程

    std::cout<< "Server start..." << std::endl;

    server.start();

    return 0;
}