#include "my_muduo/Acceptor.h"
#include "my_muduo/EventLoop.h"
#include "my_muduo/InetAddress.h"

namespace mymuduo
{

Acceptor::Acceptor(EventLoop* loop, const std::string& ip, uint16_t port)
    : loop_(loop)
    , serverSock_(Socket::createNonblocking())
    , acceptChannel_(loop_, serverSock_.fd())
{
    InetAddress serverAddr(port, ip);

    // 配置监听 Socket 选项
    serverSock_.setReuseAddr(true);
    serverSock_.setReusePort(true);
    serverSock_.setTcpNoDelay(true);
    serverSock_.setKeepAlive(true);

    // 绑定并监听
    serverSock_.bind(serverAddr);
    serverSock_.listen();

    // 设置 Channel 的可读回调为接收新连接的逻辑
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
    acceptChannel_.enableReading();
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::handleRead()
{
    InetAddress clientAddr;
    int connFd = serverSock_.accept(clientAddr);

    if (connFd >= 0)
    {
        // 封装为智能指针管理 Socket
        std::unique_ptr<Socket> clientSock(new Socket(connFd));
        clientSock->setIpPort(clientAddr.toIp(), clientAddr.toPort());

        // 执行上层（TcpServer）传入的回调函数
        if (newConnectionCallback_)
        {
            newConnectionCallback_(std::move(clientSock));
        }
    }
    else
    {
        // 这里可以处理一些特定的 accept 错误，比如 EMFILE（描述符用尽）
    }
}

} // namespace mymuduo