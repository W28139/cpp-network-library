#pragma once
#include<sys/epoll.h>
#include"Epoll.h"
#include"Socket.h"
#include"InetAddress.h"
#include"EventLoop.h"
#include<functional>
#include<memory>
class EventLoop;

class Channel
{
private:
	int fd_ = -1;
	EventLoop* loop_;
	bool inepoll_ = false;
	uint32_t events_ = 0;
	uint32_t revents_ = 0;

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
	void handleevent();

	void setreadcallback(std::function<void()>fn);
	void setclosecallback(std::function<void()>fn);
	void seterrorcallback(std::function<void()>fn);
	void setwritecallback(std::function<void()>fn);
};

