#pragma once
#include"TcpServer.h"
#include"EventLoop.h"
#include"Connection.h"

class EchoServer
{
private:
	TcpServer tcpserver_;
	ThreadPool threadpool_;		// 工作线程池

public:
	EchoServer(const std::string &ip,const uint16_t port,int subthreadnum = 3,int workthreadnum=5);
	~EchoServer();

	void Start();

	void HandleNewConnection(spConnection conn);
	void HandleClose(spConnection conn);
	void HandleError(spConnection conn);
	void HandleSendComplete(spConnection conn);
	void HandleTimeOut(EventLoop *loop);
	void HandleMessage(spConnection conn,std::string& message);
	void OnMessage(spConnection conn,std::string& message);	// 处理客户端的请求报文，用于添加给线程
};
