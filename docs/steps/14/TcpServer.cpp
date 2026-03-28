#include"TcpServer.h"
TcpServer::TcpServer(const std::string &ip,const uint16_t port)
{
	acceptor_ = new Acceptor(&loop_,ip,port);
	
	// 给 acceptor_（连接接收器）注册一个“新连接到达”时的回调函数
	acceptor_ -> setnewconnectioncb(std::bind(&TcpServer::newconnection,this,std::placeholders::_1));
}

TcpServer::~TcpServer()
{
	delete acceptor_;
}

void TcpServer::start()
{
	loop_.run();
}

void TcpServer::newconnection(Socket* clientsock)
{
	Connection *conn = new Connection(&loop_,clientsock);			
}


