#pragma once
#include"Epoll.h"

class Channel;
class Epoll;

class EventLoop
{
private:
	Epoll *ep_;

public:
	EventLoop();
	~EventLoop();

	void run();
  	// Epoll* ep(); 不需要返回 Epoll*类型，完全由 EventLoop接替
	void updatechannel(Channel* ch);
};
