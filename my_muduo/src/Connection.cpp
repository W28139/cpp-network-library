#include "my_muduo/Connection.h"
#include "my_muduo/Socket.h"
#include "my_muduo/Channel.h"
#include "my_muduo/EventLoop.h"

#include <unistd.h>
#include <sys/socket.h>

namespace mymuduo
{

Connection::Connection(EventLoop* loop, std::unique_ptr<Socket> clientSock)
    : loop_(loop)
    , socket_(std::move(clientSock))
    , channel_(new Channel(loop_, socket_->fd()))
    , disconnected_(false)
    , lastActiveTime_(Timestamp::now())
{
    // 设置 Channel 的回调函数
    channel_->setReadCallback(std::bind(&Connection::handleRead, this));
    channel_->setWriteCallback(std::bind(&Connection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&Connection::handleClose, this));
    channel_->setErrorCallback(std::bind(&Connection::handleError, this));

    // 使用 ET 模式并开启监听
    channel_->useET();
    channel_->enableReading();
}

Connection::~Connection()
{
}

int Connection::fd() const
{
    return socket_->fd();
}

std::string Connection::ip() const
{
    return socket_->ip();
}

uint16_t Connection::port() const
{
    return socket_->port();
}

void Connection::handleRead()
{
    int savedErrno = 0;
    // 从 fd 中读取数据到输入缓冲区
    ssize_t n = inputBuffer_.readFd(fd(), &savedErrno);

    if (n > 0)
    {
        // 更新最后活跃时间
        lastActiveTime_ = Timestamp::now();
        
        // 提取所有数据并回调用户定义的 onMessage
        std::string message = inputBuffer_.retrieveAllAsString();
        if (onMessageCallback_)
        {
            onMessageCallback_(shared_from_this(), message);
        }
    }
    else if (n == 0)
    {
        // 读到 0，表示对端关闭
        handleClose();
    }
    else
    {
        errno = savedErrno;
        handleError();
    }
}

void Connection::handleWrite()
{
    if (channel_->isWriting())
    {
        ssize_t n = ::send(fd(), outputBuffer_.peek(), outputBuffer_.readableBytes(), 0);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            // 如果发完了
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (sendCompleteCallback_)
                {
                    // 在所属线程执行发送完成回调
                    loop_->runInLoop(std::bind(sendCompleteCallback_, shared_from_this()));
                }
            }
        }
        else
        {
            ::fprintf(stderr, "Connection::handleWrite() failed\n");
        }
    }
}

void Connection::handleClose()
{
    if(disconnected_)
        return;

    disconnected_ = true;

    ConnectionPtr self(shared_from_this());

    loop_->runInLoop(
        [self]()
        {
            self->channel_->disableAll();
            self->channel_->remove();

            if(self->closeCallback_)
            {
                self->closeCallback_(self);
            }
        });
}

void Connection::handleError()
{
    if (!disconnected_)
    {
        disconnected_ = true;
        channel_->disableAll();
        channel_->remove();

        if (errorCallback_)
        {
            errorCallback_(shared_from_this());
        }
    }
}

void Connection::send(const std::string& data)
{
    if (disconnected_)
    {
        return;
    }

    if (loop_->isInLoopThread())
    {
        sendInLoop(data);
    }
    else
    {
        // 线程安全：如果不是在本线程调用的，则投递到本线程执行
        loop_->runInLoop(std::bind(&Connection::sendInLoop, this, data));
    }
}

void Connection::sendInLoop(const std::string& data)
{
    ssize_t nwrote = 0;
    size_t remaining = data.size();
    bool faultError = false;

    // 如果之前没有数据在排队，尝试直接发送
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::send(fd(), data.data(), data.size(), 0);
        if (nwrote >= 0)
        {
            remaining = data.size() - nwrote;
            if (remaining == 0 && sendCompleteCallback_)
            {
                loop_->runInLoop(std::bind(sendCompleteCallback_, shared_from_this()));
            }
        }
        else
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    // 如果直接发送没发完，或者之前就有数据在排队，则放入输出缓冲区并监听写事件
    if (!faultError && remaining > 0)
    {
        outputBuffer_.append(data.data() + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

bool Connection::isTimeout(time_t now, int seconds) const
{
    return (now - lastActiveTime_.toSeconds()) > seconds;
}

} // namespace mymuduo