#include"TcpServer.h"
TcpServer::TcpServer(const std::string &ip,const uint16_t port)
{
	acceptor_ = new Acceptor(&loop_,ip,port);
	
	// 给 acceptor_（连接接收器）注册一个“新连接到达”时的回调函数
	acceptor_ -> setnewconnectioncb(std::bind(&TcpServer::newconnection,this,std::placeholders::_1));
	loop_.setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));
}

TcpServer::~TcpServer()
{
	delete acceptor_;
	for(auto& con:conns_)
	{
		delete con.second;
	}
}

void TcpServer::start()
{
	loop_.run();
}

void TcpServer::newconnection(Socket* clientsock)
{
	Connection *conn = new Connection(&loop_,clientsock);			
	conn->setclosecallback(std::bind(&TcpServer::closeconnection,this,std::placeholders::_1));
	conn->seterrorcallback(std::bind(&TcpServer::errorconnection,this,std::placeholders::_1));
	conn->setonmessagecallback(std::bind(&TcpServer::onmessage,this,std::placeholders::_1,std::placeholders::_2));
	conn->setsendcompletecallback(std::bind(&TcpServer::sendcomplete,this,std::placeholders::_1));
	
	conns_[conn->fd()] = conn;
	
	// 建立后回调
	if(newconnectioncb_) newconnectioncb_(conn);
}

void TcpServer::closeconnection(Connection *conn)
{
	// 析构前回调
	if(closeconnectioncb_) closeconnectioncb_(conn);

	conns_.erase(conn->fd());
	delete conn;
}
void TcpServer::errorconnection(Connection *conn)
{
	if(errorconnectioncb_)	errorconnectioncb_(conn);
	conns_.erase(conn->fd());
	delete conn;
}

void TcpServer::onmessage(Connection *conn,std::string& message)
{
	if(onmessagecb_) onmessagecb_(conn,message);
}

void TcpServer::sendcomplete(Connection *conn)
{
	if(sendcompletecb_) sendcompletecb_(conn);
}

void TcpServer::epolltimeout(EventLoop *loop)
{
	if(timeoutcb_) timeoutcb_(loop);
}


void TcpServer::setnewconnectioncb(std::function<void(Connection*)>fn)
{
	newconnectioncb_ = fn;
}
void TcpServer::setcloseconnectioncb(std::function<void(Connection*)>fn)
{
	closeconnectioncb_ = fn;
}
void TcpServer::seterrorconnectioncb(std::function<void(Connection*)>fn)
{
	errorconnectioncb_ =fn;
}
void TcpServer::setonmessagecb(std::function<void(Connection*,std::string &message)>fn)
{
	onmessagecb_ = fn;
}
void TcpServer::setsendcompletecb(std::function<void(Connection*)>fn)
{
	sendcompletecb_ = fn;
}
void TcpServer::settimeoutcb(std::function<void(EventLoop*)>fn)
{
	timeoutcb_ = fn;
}
