#include"my_muduo/Epoll.h"

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

/*
 	updatechannel 的作用：把 Channel 挂到红黑树上，或者修改它在红黑树上的监听事件
*/
void Epoll::updatechannel(Channel *ch)
{
    struct epoll_event ev;
    // 【核心点】把 Channel 对象的指针存入 data.ptr
    // 这样当 epoll_wait 返回时，我们能直接拿到 Channel 对象，而不是干巴巴的 fd 整数
    ev.data.ptr = ch; 
    
    // 从 Channel 对象中获取它关心的事件（如 EPOLLIN | EPOLLET 等）
    ev.events = ch->events();
    
    // 判断这个 Channel 是否已经在 epoll 的红黑树里了
    if(ch->inepoll())
    {
        // 情况 A：已经在树上了，说明这次是来修改监听事件的（MOD）
        // 比如：之前只读，现在要发数据了，需要增加对可写事件的监听
        int ret = epoll_ctl(epollfd_, EPOLL_CTL_MOD, ch->fd(), &ev);
        if(ret == -1)
        {
            perror("epoll_ctl MOD failed");
            exit(-1);
        }
    }
    else
    {
        // 情况 B：不在树上，说明是一个新连接，需要把它加到红黑树里（ADD）
        int ret = epoll_ctl(epollfd_, EPOLL_CTL_ADD, ch->fd(), &ev);
        if(ret == -1)
        {
            perror("epoll_ctl ADD failed");
            exit(-1);
        }
        // 更新 Channel 内部的状态位，标记它现在已经“在树上”了
        ch->setinepoll(); 
    }
}

/*
  removechannel 的作用：把 Channel 从红黑树上摘除
 */
void Epoll::removechannel(Channel* ch)
{
    // 只有在树上的 Channel 才能被删除
    if(ch->inepoll())
    {
        printf("removechannel(). fd=%d\n", ch->fd());
        
        // 调用 epoll_ctl 进行删除（DEL）
        // 对于 DEL 操作，最后一个参数 ev 在 2.6.9 版本之后的内核中可以传 0/NULL
        int ret = epoll_ctl(epollfd_, EPOLL_CTL_DEL, ch->fd(), 0);
        if(ret == -1)
        {
            perror("epoll_ctl DEL failed.\n");
            exit(-1);
        }
        
        /* 
           注意：这里通常还需要调用 ch->setinepoll(false) 
           或者在 Channel 销毁前确保状态同步。
        */
    }
}