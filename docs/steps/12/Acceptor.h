#pragma once
#include<functional>
#include"Socket.h"
#include"InetAddress.h"
#include"Channel.h"
#include"EventLoop.h"

class Acceptor
{
private:
	EventLoop* loop_;	// Acceptor对应的事件循环，在构造函数中传入
	Socket* serversock_;	// 服务端用于监听的socket,在构造函数中创建
	Channel* acceptchannel_;// Acceptor对应的channel，在构造函数中创建
	
public:
	Acceptor(EventLoop *loop,const std::string &ip,const uint16_t port);
	~Acceptor();
};

