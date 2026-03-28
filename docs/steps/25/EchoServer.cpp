#include"EchoServer.h"

EchoServer::EchoServer(const std::string &ip,const uint16_t port,int subthreadnum,int workthreadnum)
	:tcpserver_(ip,port,subthreadnum),threadpool_(workthreadnum,"WORKS")
{
	tcpserver_.setnewconnectioncb(std::bind(&EchoServer::HandleNewConnection,this,std::placeholders::_1));
	tcpserver_.setcloseconnectioncb(std::bind(&EchoServer::HandleClose,this,std::placeholders::_1));
	tcpserver_.seterrorconnectioncb(std::bind(&EchoServer::HandleError,this,std::placeholders::_1));
	tcpserver_.setonmessagecb(std::bind(&EchoServer::HandleMessage,this,std::placeholders::_1,std::placeholders::_2));
	tcpserver_.setsendcompletecb(std::bind(&EchoServer::HandleSendComplete,this,std::placeholders::_1));
	tcpserver_.settimeoutcb(std::bind(&EchoServer::HandleTimeOut,this,std::placeholders::_1));
}

EchoServer::~EchoServer(){}

void EchoServer::Start()
{
	tcpserver_.start();
}

void EchoServer::HandleNewConnection(spConnection conn)
{
	std::cout<<"New Connection Come in."<<std::endl;
	// 根据业务需求，补充代码
}
void EchoServer::HandleClose(spConnection conn)
{
	std::cout<<"New Connection Come out"<<std::endl;
        // 根据业务需求，补充代码
}
void EchoServer::HandleError(spConnection conn)
{
	std::cout<<"EchoServer conn error"<<std::endl;
        // 根据业务需求，补充代码
}
void EchoServer::HandleSendComplete(spConnection conn)
{
	std::cout<<"Message send complete."<<std::endl;
        // 根据业务需求，补充代码
}
void EchoServer::HandleTimeOut(EventLoop *loop)
{
	std::cout<<"EchoServer timeout."<<std::endl;
        // 根据业务需求，补充代码
}
void EchoServer::OnMessage(spConnection conn,std::string &message)
{
	message = "repay:" + message;
	conn->send(message.data());
	
}
void EchoServer::HandleMessage(spConnection conn,std::string &message)
{
	if(threadpool_.size()==0)
	{
		// 不使用工作线程
		OnMessage(conn,message);
	}
	else
	{
		// 把业务处理的函数，添加到线程池任务队列中
		threadpool_.addtask(std::bind(&EchoServer::OnMessage,this,conn,message));
	}
}