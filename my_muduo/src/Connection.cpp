#include"my_muduo/Connection.h"

Connection::Connection(EventLoop* loop,std::unique_ptr<Socket>clientsock)
	:loop_(loop),
	clientsock_(std::move(clientsock)),
	disconnect_(false),
	clientchannel_(new Channel(loop_,clientsock_->fd())),
	inputbuffer_(1024),  // 初始化新 Buffer，给个初始大小
    outputbuffer_(1024) 
{
	// clientchannel_ = new Channel(loop_,clientsock_->fd());
	clientchannel_->setreadcallback(std::bind(&Connection::onmessage,this));
	clientchannel_->setclosecallback(std::bind(&Connection::closecallback,this));
	clientchannel_->seterrorcallback(std::bind(&Connection::errorcallback,this));
	clientchannel_->setwritecallback(std::bind(&Connection::writecallback,this));
	clientchannel_->useet();
	clientchannel_->enablereading();

}

Connection::~Connection()
{
	// delete clientchannel_;
	// delete clientsock_;
	// printf("该连接已清除\n");
}

int Connection::fd() const
{
	return clientsock_->fd();
}

std::string Connection::ip() const
{
	return clientsock_->ip();
}

uint16_t Connection::port() const
{
	return clientsock_->port();
}

void Connection::setclosecallback(std::function<void(spConnection)>fn)
{
	closecallback_ = fn;
}

void Connection::seterrorcallback(std::function<void(spConnection)>fn)
{
	errorcallback_ = fn;
}

void Connection::setonmessagecallback(std::function<void(spConnection,std::string&)>fn)
{
	onmessagecallback_ = fn; 
}

void Connection::setsendcompletecallback(std::function<void(spConnection)>fn)
{
	sendcompletecallback_ = fn;
}

void Connection::closecallback()
{
	disconnect_ = true;
	clientchannel_->remove();
	closecallback_(shared_from_this());
}
void Connection::errorcallback()
{
	disconnect_ = true;
	clientchannel_->remove();
	errorcallback_(shared_from_this());	
}

void Connection::onmessage()
{
    int savedErrno = 0;
    // 1. 直接调用新 Buffer 的核心函数，利用 readv 散射读取提升性能
    ssize_t n = inputbuffer_.readFd(fd(), &savedErrno);

    if (n > 0)
    {
        // 2. 根据你的业务需求提取数据。
        // 测试 HTTP 或简单 Echo 时，通常直接取出所有数据并清空
        std::string message = inputbuffer_.retrieveAllAsString();
        
        // 3. 更新时间戳并执行回调
        lastatime_ = Timestamp::now();
        onmessagecallback_(shared_from_this(), message);
    }
    else if (n == 0)
    {
        // 客户端关闭连接
        closecallback();
    }
    else
    {
        // 错误处理
        errno = savedErrno;
        errorcallback();
    }
}

void Connection::writecallback()
{
    // 1. 获取当前缓冲区里有多少可读（可发送）数据
    size_t n_to_write = outputbuffer_.readableBytes();
    
    // 2. 调用系统底层 send，从 peek() 指向的位置开始发送
    ssize_t n_written = ::send(fd(), outputbuffer_.peek(), n_to_write, 0);

    if (n_written > 0)
    {
        // 3. 【关键】发送了多少，就从 Buffer 中移除（移动 readerIndex_）多少
        outputbuffer_.retrieve(n_written);
    }

    // 4. 如果缓冲区发空了，停止监听写事件
    if (outputbuffer_.readableBytes() == 0)
    {
        clientchannel_->disablewriting();
        if (sendcompletecallback_) {
            sendcompletecallback_(shared_from_this());
        }
    }
}

void Connection::send(std::string data)
{

	if(disconnect_ == true)
	{
		printf("客户端已经断开连接，send()直接返回。\n");
		return;
	}
	if(loop_->isinloopthread())
	{
		// 如果当前线程是IO线程，直接执行发送数据操作
		// printf("send() 在事件循环的线程中。\n");
		sendinloop(data);
	}
	else
	{
		// 如果当前线程不是IO线程，把发送数据的操作交给IO线程去执行
		// printf("send() 不在事件循环的线程中。\n");
		loop_->queueinloop(std::bind(&Connection::sendinloop,this,std::move(data)));
	}
}

// 这个是IO事件
void Connection::sendinloop(const std::string& data)
{
    // 不再根据 seq 判断，直接将原始字节流追加到输出缓冲区
    outputbuffer_.append(data.data(), data.size());
    
    // 注册写事件，触发 writecallback 进行真正发送
    clientchannel_->enablewriting();
}

bool Connection::timeout(time_t now,int val)
{
	return now - lastatime_.toint() > val;
}
