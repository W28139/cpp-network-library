#pragma once
#include<sys/epoll.h>
#include"Epoll.h"
#include"Socket.h"
#include"InetAddress.h"
#include"EventLoop.h"
#include<functional>
#include<memory>
class EventLoop;

// Channel 的核心意义是：它把文件描述符（fd）和它关心的事件（events）以及处理函数（callbacks）绑定在了一起
class Channel
{
private:
	int fd_ = -1;
	EventLoop* loop_;
	bool inepoll_ = false;
	uint32_t events_ = 0;	// 关心的事件
	uint32_t revents_ = 0;	// 实际发生的事件

	std::function<void()>readcallback_;
	std::function<void()>closecallback_;	// 关闭fd_的回调函数，将回调Connection::closecallback()
	std::function<void()>errorcallback_;	// fd_发生了错误的回调函数，回调Connection::errorcallback()
	std::function<void()>writecallback_;

public:
	Channel(EventLoop* loop,int fd);
	~Channel();

	int fd();
	void useet();
	
	void enablereading();
	void disablereading();
	void enablewriting();
	void disablewriting();
	
	void disableall();	// 取消写事件
	void remove();		// 取消全部事件

	void setinepoll();
	void setrevents(uint32_t ev);
	bool inepoll();
	uint32_t events();
	uint32_t revents();
	// handleevent() 根据 revents_ 是什么，去调用对应的 readcallback_ 或 writecallback_。
	void handleevent();

	void setreadcallback(std::function<void()>fn);
	void setclosecallback(std::function<void()>fn);
	void seterrorcallback(std::function<void()>fn);
	void setwritecallback(std::function<void()>fn);
};

