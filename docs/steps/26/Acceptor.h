#pragma once
#include<functional>
#include"Socket.h"
#include"InetAddress.h"
#include"Channel.h"
#include"EventLoop.h"
#include<memory>
class Acceptor
{
private:
	EventLoop* loop_;	
	Socket serversock_;
	Channel acceptchannel_;
	
	// 处理新客户端连接请求的回调函数，将指向TcpServer::newconnection()
	std::function<void(std::unique_ptr<Socket>)>newconnectioncb_;
	
public:
	Acceptor(EventLoop* loop,const std::string &ip,const uint16_t port);
	~Acceptor();

	void newconnection();
	// 设置处理新客户端连接请求的回调函数
	void setnewconnectioncb(std::function<void(std::unique_ptr<Socket>)> fn);
};


