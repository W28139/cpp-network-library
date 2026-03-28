#pragma once
#include"Epoll.h"
#include<functional>
#include<memory>
#include<unistd.h>
#include<queue>
#include<mutex>
#include <sys/eventfd.h>
#include<sys/syscall.h>
#include<sys/timerfd.h>	// 定时器需要包含这个头文件
#include"Connection.h"
#include<map>
class Channel;
class Epoll;
class Connection;
using spConnection = std::shared_ptr<Connection>;

class EventLoop
{
private:
	std::unique_ptr<Epoll> ep_;
	std::function<void(EventLoop*)>epolltimeoutcallback_;
	pid_t threadid_;				// 事件循环所在线程的id
	std::queue<std::function<void()>>taskqueue_;	// 事件循环线程被eventfd唤醒后执行的任务队列
	int timetvl_;		// 闹钟时间间隔，单位：秒
	int timeout_;		// Connection对象超时的事件
	std::mutex mutex_;				// 任务队列同步的互斥锁
	int wakeupfd_;					// 用于唤醒事件循环线程的fd
	std::unique_ptr<Channel> wakechannel_;		// 用于唤醒事件循环线程的eventfd
	int timerfd_;				// 定时器的fd
	std::unique_ptr<Channel> timerchannel_;	// 定时器的Channel
	std::mutex mmutex_;			// 保护conns_的互斥锁			
	bool mainloop_;	// 主ture 从false
	
	std::map<int,spConnection>conns_;	// 存放运行在该事件循环上的全部Connection
	std::function<void(int)> timercallback_;


public:
	EventLoop(bool mainloop,int timetvl = 30,int timeout = 100);
	~EventLoop();

	void run();
	void updatechannel(Channel* ch);
	void removechannel(Channel *ch);
	void setepolltimeoutcallback(std::function<void(EventLoop*)>fn);
	
	bool isinloopthread();

	void queueinloop(std::function<void()>fn);	// 把任务添加到任务队列中
	void wakeup();					// 用eventfd唤醒事件循环线程
	void handlewakeup();
	void handletimer();				// 闹钟响时执行的函数
	
	void newconnection(spConnection conn);		// 把Connection对象存放在conns_中
	void settimercallback(std::function<void(int)>fn);
};
