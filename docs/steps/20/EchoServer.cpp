#include"EchoServer.h"

EchoServer::EchoServer(const std::string &ip,const uint16_t port):tcpserver_(ip,port)
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

void EchoServer::HandleNewConnection(Connection *conn)
{
	std::cout<<"New Connection Come in."<<std::endl;
	// 根据业务需求，补充代码
}
void EchoServer::HandleClose(Connection *conn)
{
	std::cout<<"New Connection Come out"<<std::endl;
        // 根据业务需求，补充代码
}
void EchoServer::HandleError(Connection *conn)
{
	std::cout<<"EchoServer conn error"<<std::endl;
        // 根据业务需求，补充代码
}
void EchoServer::HandleSendComplete(Connection *conn)
{
	std::cout<<"Message send complete."<<std::endl;
        // 根据业务需求，补充代码
}
void EchoServer::HandleTimeOut(EventLoop *loop)
{
	std::cout<<"EchoServer timeout."<<std::endl;
        // 根据业务需求，补充代码
}
void EchoServer::HandleMessage(Connection *conn,std::string &message)
{
	message = "repay:" + message;

	conn->send(message.data(),message.size());
}
