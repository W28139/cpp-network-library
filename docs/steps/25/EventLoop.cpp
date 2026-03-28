#include"EventLoop.h"
EventLoop::EventLoop()
	:ep_(new Epoll),wakeupfd_(eventfd(0,EFD_NONBLOCK)),wakechannel_(new Channel(this,wakeupfd_))
{
	wakechannel_->setreadcallback(std::bind(&EventLoop::handlewakeup,this));
	wakechannel_->enablereading();
}

EventLoop::~EventLoop()
{
	// delete ep_;
}

void EventLoop::run()
{
	// run一定发生在主事件循环里，即线程ID一定是IO线程的ID
	threadid_ = syscall(SYS_gettid);	// 获取事件循环所在线程的ID
	while(1)
	{
		std::vector<Channel*> channels = ep_->loop();
	
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

void EventLoop::removechannel(Channel* ch)
{
	ep_->removechannel(ch);
}

void EventLoop::setepolltimeoutcallback(std::function<void(EventLoop*)>fn)
{
	epolltimeoutcallback_ = fn;
}

bool EventLoop::isinloopthread()
{
	return threadid_ == syscall(SYS_gettid);
}

void EventLoop::queueinloop(std::function<void()>fn)
{
	{
		std::lock_guard<std::mutex>gd(mutex_);
		taskqueue_.push(fn);
	}
	// 唤醒事件循环
	wakeup();
}


void EventLoop::wakeup()
{
	uint64_t val = 1;
	write(wakeupfd_,&val,sizeof(val));
}

void EventLoop::handlewakeup()
{
	printf("handlewakeup(*) thread id is %ld.\n",syscall(SYS_gettid));

	uint64_t val;
	read(wakeupfd_,&val,sizeof(val));	// 从eventfd里读出来，如果不读，eventfd的读事件会一直触发（水平触发）
	
	std::function<void()>fn;
	std::lock_guard<std::mutex>gd(mutex_);	// 给任务队列加锁
	while(taskqueue_.size()>0)
	{
		fn = std::move(taskqueue_.front());
		taskqueue_.pop();
		fn();				// 执行任务
	}
}	
