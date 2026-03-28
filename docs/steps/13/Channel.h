#pragma once
#include<sys/epoll.h>
#include"Epoll.h"
#include"Socket.h"
#include"InetAddress.h"
#include"EventLoop.h"
#include<functional>

class EventLoop;

class Channel
{
private:
	int fd_ = -1;
	
	// Epoll* ep_ = nullptr;
	EventLoop *loop_ = nullptr;
	
	bool inepoll_ = false;
	uint32_t events_ = 0;
	uint32_t revents_ = 0;

	std::function<void()>readcallback_;

public:
	Channel(EventLoop* loop,int fd);
	~Channel();

	int fd();
	void useet();
	void enablereading();
	void setinepoll();
	void setrevents(uint32_t ev);
	bool inepoll();
	uint32_t events();
	uint32_t revents();
	void handleevent();
	void onmessage();

	void setreadcallback(std::function<void()>fn);
};
