#include"Connection.h"

Connection::Connection(EventLoop* loop,std::unique_ptr<Socket>clientsock)
	:loop_(loop),clientsock_(std::move(clientsock)),disconnect_(false),clientchannel_(new Channel(loop_,clientsock_->fd()))
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

void Connection::writecallback()
{
	// 尝试把发送缓存区的全部数据发送出去
	int writen = ::send(fd(),outputbuffer_.data(),outputbuffer_.size(),0);

	if(writen>0)
	{
		outputbuffer_.erase(0,writen);	// 删除outputbuffer_里已发送出的数据
	}
	if(outputbuffer_.size()==0)
	{
		clientchannel_->disablewriting();// 如果数据为0了，那就关闭监听可写事件，避免一致返回可写事件
		sendcompletecallback_(shared_from_this());
	}
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


void Connection::onmessage()
{
	char buffer[1024];
	while(1)
	{
		bzero(&buffer,sizeof(buffer));
		ssize_t nread = read(fd(),buffer,sizeof(buffer));

		if(nread>0)
		{
			inputbuffer_.append(buffer,nread);
		}
		else if(nread == -1 && errno == EINTR)
		{
			continue;
		}
		else if(nread==-1 &&((errno==EAGAIN)||(errno==EWOULDBLOCK)))	// 读取完全部数据
		{
			std::string message;
			while(1)
			{
				if(inputbuffer_.pickmessage(message) == false)
				{
					break;
				}
				// printf("message(eventfd=%d):%s.\n",fd(),message.c_str());
				lastatime_ = Timestamp::now();		// 更新Connection时间戳
				// std::cout<<"lastatime = "<<lastatime_.tostring()<<std::endl;

				onmessagecallback_(shared_from_this(),message);
			}
			break;
		}
		else if(nread == 0)
		{
			// clientchannel_->remove();
			closecallback();
			break;	
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
	outputbuffer_.appendwithseq(data.data(),data.size());
	clientchannel_->enablewriting();
}

bool Connection::timeout(time_t now,int val)
{
	return now - lastatime_.toint() > val;
}
