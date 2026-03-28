#pragma once
#include<functional>
#include"Socket.h"
#include"InetAddress.h"
#include"Channel.h"
#include"EventLoop.h"
#include<atomic>
#include"Buffer.h"
#include<memory>
class Connection;
using spConnection = std::shared_ptr<Connection>;

class Connection:public std::enable_shared_from_this<Connection>
{
private:
	const std::unique_ptr<EventLoop> &loop_;
	std::unique_ptr<Socket> clientsock_;
	std::unique_ptr<Channel> clientchannel_;

	Buffer inputbuffer_;
	Buffer outputbuffer_;
	
	std::atomic_bool disconnect_;	// 客户端连接是否已断开，如果已断开，则设置为true

	std::function<void(spConnection)>closecallback_;
	std::function<void(spConnection)>errorcallback_;
	std::function<void(spConnection,std::string&)>onmessagecallback_;
	std::function<void(spConnection)>sendcompletecallback_;
	
public:
	Connection(const std::unique_ptr<EventLoop> &loop,std::unique_ptr<Socket> clientsock);
	~Connection();

	int fd() const;
	std::string ip() const;
	uint16_t port() const;

	void closecallback();
	void errorcallback();
	void writecallback();

	void setclosecallback(std::function<void(spConnection)>fn);
	void seterrorcallback(std::function<void(spConnection)>fn);
	void setonmessagecallback(std::function<void(spConnection,std::string&)>fn);
	void setsendcompletecallback(std::function<void(spConnection)>fn);

	void onmessage();

	void send(const char* data,size_t size);
};

