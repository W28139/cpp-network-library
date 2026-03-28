#include"Channel.h"
#include "Connection.h"

Channel::Channel(EventLoop* loop,int fd):loop_(loop),fd_(fd){}

Channel::~Channel(){}

int Channel::fd()
{
	return fd_;
}

void Channel::useet()
{
	events_ = events_|EPOLLET;
}

void Channel::enablereading()
{
	events_ = events_|EPOLLIN;
	loop_->updatechannel(this);
}

void Channel::setinepoll()
{
	inepoll_ =true;
}

void Channel::setrevents(uint32_t ev)
{
	revents_ = ev;
}

bool Channel::inepoll()
{
	return inepoll_;
}

uint32_t Channel::events()
{
	return events_;
}

uint32_t Channel::revents()
{
	return revents_;
}

void Channel::handleevent()
{
	if(revents_ & EPOLLRDHUP)
	{
		printf("client(eventfd = %d) disconnected.\n",fd_);
		close(fd_);
	}
	else if(revents_ & (EPOLLIN|EPOLLPRI))
	{
		/*
	      	if(islisten_==true)
		{
			newconnection(serversock);
		}
		else
		{
			onmessage();
		}
		*/
		readcallback_();

	}
	else if(revents_ & EPOLLOUT)
	{
	}
	else
	{
		printf("client(eventfd=%d)error.\n",fd_);
		close(fd_);
	}	
}

void Channel::newconnection(Socket* serversock)
{
	InetAddress clientaddr;
	Socket* clientsock = new Socket(serversock->accept(clientaddr));

	printf("accept client(fd=%d,ip=%s,port=%d) ok.\n",
		clientsock->fd(),clientaddr.ip(),clientaddr.port());

	Connection *conn = new Connection(loop_,clientsock);			
}

void Channel::onmessage()
{
	char recv_buf[1024];
	while(1)
	{
		bzero(&recv_buf,sizeof(recv_buf));
		ssize_t nread = read(fd_,recv_buf,sizeof(recv_buf));

		if(nread>0)
		{
			printf("recv(eventfd=%d):%s\n",fd_,recv_buf);
			write(fd_,recv_buf,nread);
		}
		else if(nread == -1 && errno == EINTR)
		{
			continue;
		}
		else if(nread==-1 &&((errno==EAGAIN)||(errno==EWOULDBLOCK)))
		{
			break;
		}
		else if(nread == 0)
		{
			printf("client(eventfd=%d)disconnected.\n",fd_);
			close(fd_);
			break;
		}
	}

}

void Channel::setreadcallback(std::function<void()>fn)
{
	readcallback_ = fn;
}

