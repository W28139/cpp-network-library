#include"Connection.h"

Connection::Connection(EventLoop *loop,Socket *clientsock):loop_(loop),clientsock_(clientsock)
{
	clientchannel_ = new Channel(loop_,clientsock_->fd());
	// clientchannel_->setreadcallback(std::bind(&Channel::onmessage,clientchannel_));
	clientchannel_->setreadcallback(std::bind(&Connection::onmessage,this));

	clientchannel_->useet();
	clientchannel_->enablereading();

	clientchannel_->setclosecallback(std::bind(&Connection::closecallback,this));
	clientchannel_->seterrorcallback(std::bind(&Connection::errorcallback,this));
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

void Connection::setclosecallback(std::function<void(Connection*)>fn)
{
	closecallback_ = fn;
}

void Connection::seterrorcallback(std::function<void(Connection*)>fn)
{
	errorcallback_ = fn;
}

void Connection::setonmessagecallback(std::function<void(Connection*,std::string)>fn)
{
	onmessagecallback_ = fn;
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
