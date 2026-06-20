#include "my_muduo/Socket.h"
#include "my_muduo/InetAddress.h"

#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>

namespace mymuduo
{

int Socket::createNonblocking()
{
    // 创建非阻塞、带 CLOEXEC 标志的套接字
    int listenFd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (listenFd < 0)
    {
        ::fprintf(stderr, "socket create failed: %d\n", errno);
    }
    return listenFd;
}

Socket::Socket(int fd)
    : fd_(fd)
{
}

Socket::~Socket()
{
    // RAII 自动关闭描述符
    ::close(fd_);
}

void Socket::setIpPort(const std::string& ip, uint16_t port)
{
    ip_ = ip;
    port_ = port;
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}

void Socket::bind(const InetAddress& serverAddr)
{
    int ret = ::bind(fd_, serverAddr.getSockAddr(), sizeof(struct sockaddr_in));
    if (ret < 0)
    {
        ::fprintf(stderr, "bind fd:%d to %s failed, errno:%d\n", fd_, serverAddr.toIpPort().c_str(), errno);
    }
}

void Socket::listen(int backlog)
{
    int ret = ::listen(fd_, backlog);
    if (ret < 0)
    {
        ::fprintf(stderr, "listen fd:%d failed, errno:%d\n", fd_, errno);
    }
}

int Socket::accept(InetAddress& clientAddr)
{
    struct sockaddr_in addr;
    ::memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);

    // 使用 accept4 一步到位地设置非阻塞和 CLOEXEC
    int connFd = ::accept4(fd_, (struct sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connFd >= 0)
    {
        clientAddr.setSockAddr(addr);
    }
    else
    {
        ::fprintf(stderr, "accept failed, errno:%d\n", errno);
    }
    return connFd;
}

} // namespace mymuduo