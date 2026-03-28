#include"Channel.h"

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
	events_ |= EPOLLIN;	// 注册读事件
	loop_->updatechannel(this);
}

void Channel::disablereading()
{
	events_ &= ~EPOLLIN;	// 取消读时间
	loop_->updatechannel(this);

}
void Channel::enablewriting()
{
	events_ |= EPOLLOUT;	// 注册写事件
	loop_->updatechannel(this);
}

void Channel::disablewriting()
{
	events_ &= ~EPOLLOUT;	// 取消写事件
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
		printf("EPOLLRDHUP\n");
		closecallback_();
	}
	else if(revents_ & (EPOLLIN|EPOLLPRI))
	{
		printf("EPOLLIN|EPOLLPRI\n");
		readcallback_();

	}
	else if(revents_ & EPOLLOUT)
	{
		printf("EPOLLOUT\n");
		writecallback_();

	}
	else
	{
		printf("ERROR\n");
		errorcallback_();
	}	
}



void Channel::setreadcallback(std::function<void()>fn)
{
	readcallback_ = fn;
}


void Channel::setclosecallback(std::function<void()>fn)
{
	closecallback_ = fn;
}


void Channel::seterrorcallback(std::function<void()>fn)
{
	errorcallback_ = fn;
}

void Channel::setwritecallback(std::function<void()>fn)
{
	writecallback_ = fn;
}
