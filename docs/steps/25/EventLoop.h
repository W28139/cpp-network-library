#pragma once
#include"Epoll.h"
#include<functional>
#include<memory>
#include<unistd.h>
#include<queue>
#include<mutex>
#include <sys/eventfd.h>
#include<sys/syscall.h>
class Channel;
class Epoll;

class EventLoop
{
private:
	std::unique_ptr<Epoll> ep_;
	std::function<void(EventLoop*)>epolltimeoutcallback_;
	pid_t threadid_;				// 事件循环所在线程的id
	std::queue<std::function<void()>>taskqueue_;	// 事件循环线程被eventfd唤醒后执行的任务队列
	std::mutex mutex_;				// 任务队列同步的互斥锁
	int wakeupfd_;					// 用于唤醒事件循环线程的fd
	std::unique_ptr<Channel> wakechannel_;		// 用于唤醒事件循环线程的eventfd
public:
	EventLoop();
	~EventLoop();

	void run();
	void updatechannel(Channel* ch);
	void removechannel(Channel *ch);
	void setepolltimeoutcallback(std::function<void(EventLoop*)>fn);
	
	bool isinloopthread();

	void queueinloop(std::function<void()>fn);	// 把任务添加到任务队列中
	void wakeup();					// 用eventfd唤醒事件循环线程
	void handlewakeup();
};
