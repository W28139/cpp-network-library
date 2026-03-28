#include"EventLoop.h"

// 创建一个定时器文件描述符并设置单次超时
int createtimerfd(int sec = 30)
{
	// 创建定时器句柄，单调时间，自动关闭，非阻塞
	int tfd = timerfd_create(CLOCK_MONOTONIC,TFD_CLOEXEC|TFD_NONBLOCK);
	struct itimerspec timeout;
	memset(&timeout,0,sizeof(struct itimerspec));	
	timeout.it_value.tv_sec = sec;		// 定时时间，设置为sec
	timeout.it_value.tv_nsec = 0;		// 纳秒位数
	timerfd_settime(tfd,0,&timeout,0);	// 设置到句柄上，启动定时器
	return tfd;				// 返回建好的定时器文件描述符
}

EventLoop::EventLoop(bool mainloop,int timetvl, int timeout)
	:ep_(new Epoll),
	mainloop_(mainloop),
	timetvl_(timetvl),
	timeout_(timeout),
	wakeupfd_(eventfd(0,EFD_NONBLOCK)),
	wakechannel_(new Channel(this,wakeupfd_)),
	timerfd_(createtimerfd(timeout_)),
	timerchannel_(new Channel(this,timerfd_)),
	stop_(false)
{
	// 下面虽然都是绑的readcallback，但不是同一个channel，也就是同一颗树下不同叶子参数相同信号，做出的回应是不同
	wakechannel_->setreadcallback(std::bind(&EventLoop::handlewakeup,this));
	wakechannel_->enablereading();
	
	timerchannel_->setreadcallback(std::bind(&EventLoop::handletimer,this));
	timerchannel_->enablereading();
}

EventLoop::~EventLoop()
{
	// delete ep_;
}

void EventLoop::run()
{
	// run一定发生在主事件循环里，即线程ID一定是IO线程的ID
	threadid_ = syscall(SYS_gettid);	// 获取事件循环所在线程的ID
	while(stop_ == false)
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

void EventLoop::stop()
{
	stop_ = true;
	wakeup();
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
	// printf("handlewakeup() thread id is %ld.\n",syscall(SYS_gettid));

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

void EventLoop::handletimer()
{
	// 重新计时
	struct itimerspec timeout;
	memset(&timeout,0,sizeof(struct itimerspec));	
	timeout.it_value.tv_sec = timetvl_;		// 定时时间，设置为sec
	timeout.it_value.tv_nsec = 0;		// 纳秒位数
	timerfd_settime(timerfd_,0,&timeout,0);	// 设置到句柄上，启动定时器

	if(mainloop_)
	{
		// printf("主事件循环的闹钟时间到了");
	}
	else
	{
		// printf("从事件循环的闹钟时间到了");
		// printf("EventLoop::handletimer() thread is %ld.fd",syscall(SYS_gettid));
		time_t now = time(0);
		std::vector<int> to_delete; // 记录要踢掉的连接fd
		for (auto& aa : conns_) {
			if (aa.second->timeout(now,timeout_)) {
				to_delete.push_back(aa.first);
			}
		}
		// 统一删除
		for (int fd : to_delete) {
			// std::cout<<" "<<fd;
			{
				std::lock_guard<std::mutex>gd(mmutex_);
				conns_.erase(fd);
			}
			timercallback_(fd);
		}
		// std::cout<<std::endl;
	} 
}

void EventLoop::newconnection(spConnection conn)
{
	std::lock_guard<std::mutex>gd(mmutex_);
	conns_[conn->fd()] = conn;
}

void EventLoop::settimercallback(std::function<void(int)>fn)
{
	timercallback_ = fn;
}

