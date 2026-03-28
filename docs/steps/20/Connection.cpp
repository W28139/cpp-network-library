#include"Connection.h"

Connection::Connection(EventLoop *loop,Socket *clientsock):loop_(loop),clientsock_(clientsock)
{
	clientchannel_ = new Channel(loop_,clientsock_->fd());
	clientchannel_->setreadcallback(std::bind(&Connection::onmessage,this));
	clientchannel_->setclosecallback(std::bind(&Connection::closecallback,this));
	clientchannel_->seterrorcallback(std::bind(&Connection::errorcallback,this));
	clientchannel_->setwritecallback(std::bind(&Connection::writecallback,this));
	clientchannel_->useet();
	clientchannel_->enablereading();

}

Connection::~Connection()
{
	delete clientchannel_;
	delete clientsock_;
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
	// printf("client(eventfd=%d)disconnectioned.\n",fd());
        // close(fd());		
	closecallback_(this);
}
void Connection::errorcallback()
{
	// printf("client(eventfd=%d)error.\n",fd());
        // close(fd());	
	errorcallback_(this);	
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
		sendcompletecallback_(this);
	}
}


void Connection::setclosecallback(std::function<void(Connection*)>fn)
{
	closecallback_ = fn;
}

void Connection::seterrorcallback(std::function<void(Connection*)>fn)
{
	errorcallback_ = fn;
}

void Connection::setonmessagecallback(std::function<void(Connection*,std::string&)>fn)
{
	onmessagecallback_ = fn;
}

void Connection::setsendcompletecallback(std::function<void(Connection*)>fn)
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
			while(1)
			{
				int len;
				memcpy(&len,inputbuffer_.data(),4);
				if(inputbuffer_.size()<len+4)
					break;
				
				std::string message(inputbuffer_.data()+4,len);	
				inputbuffer_.erase(0,len+4);	
				printf("message(eventfd=%d):%s.\n",fd(),message.c_str());
				
				onmessagecallback_(this,message);
			}
			break;
		}
		else if(nread == 0)
		{
			closecallback();
			break;	
		}
	}
}


void Connection::send(const char* data,size_t size)
{
	outputbuffer_.appendwithhead(data,size);	// 把需要发送的数据保存到connection的发送缓冲区中
	clientchannel_->enablewriting();	// 注册写事件
}


