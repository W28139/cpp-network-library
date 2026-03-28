#include"EventLoop.h"
EventLoop::EventLoop():ep_(new Epoll){}

EventLoop::~EventLoop()
{
	delete ep_;
}

void EventLoop::run()
{
	while(1)
	{
		std::vector<Channel*> channels = ep_->loop(10*1000);
	
		// 此处加一个判断，channels是否为空
		// 如果为空，表示超时，回调TcpServer::epolltimeout()
		if(channels.size()==0)
		{
			epolltimeoutcallback_(this);
		}
		else
		{
			for(auto &ch:channels)
			{
				ch->handleevent();	
			}
		}
	}
}

void EventLoop::updatechannel(Channel *ch)
{
	ep_->updatechannel(ch);
}


void EventLoop::setepolltimeoutcallback(std::function<void(EventLoop*)>fn)
{
	epolltimeoutcallback_ = fn;
}

