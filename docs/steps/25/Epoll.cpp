#include"Epoll.h"

Epoll::Epoll()
{
	epollfd_ = epoll_create(1);
	if(epollfd_<0)
	{
		printf("epoll_create() failed(%d).\n",errno);
		exit(-1);
	}
}

Epoll::~Epoll()
{
	close(epollfd_);
}

void Epoll::updatechannel(Channel *ch)
{
	struct epoll_event ev;
	ev.data.ptr = ch;
	ev.events = ch->events();
	
	if(ch->inepoll())
	{
		int ret = epoll_ctl(epollfd_,EPOLL_CTL_MOD,ch->fd(),&ev);
		if(ret==-1)
		{
			perror("epoll_ctl");
			exit(-1);
		}
	}
	else
	{
		int ret = epoll_ctl(epollfd_,EPOLL_CTL_ADD,ch->fd(),&ev);
		if(ret==-1)
		{
			perror("epoll_ctl");
			exit(-1);
		}
	}
	ch->setinepoll();
}

std::vector<Channel*> Epoll::loop(int timeout)
{
	// 改为Channel*类型
	std::vector<Channel*>channels;
	bzero(events_,sizeof(events_));

	int infds = epoll_wait(epollfd_,events_,MaxEvents,timeout);
	if(infds<0)
	{
		perror("epoll_wait()");
		exit(-1);
	}
	if(infds==0)
	{
		return channels;
	}
	for(int i=0;i<infds;i++)
	{
		// 这里先取出有事件发生的ch(相当于fd,因为一一对应)
		Channel* ch = (Channel *)events_[i].data.ptr;
		// 在对应的ch里记录下所发生的事件，这也就是revents_的作用了
		ch->setrevents(events_[i].events);
		// 把所有有事件发生的events_(channel)打包返回
		channels.push_back(ch);
	}
	return channels;
}

void Epoll::removechannel(Channel* ch)
{
	if(ch->inepoll())
	{
		printf("removechannel().\n");
		int ret = epoll_ctl(epollfd_,EPOLL_CTL_DEL,ch->fd(),0);
		if(ret==-1)
		{
			perror("epoll_ctl() failed.\n");
			exit(-1);
		}
	}
}
