#include"TcpServer.h"
TcpServer::TcpServer(const std::string &ip,const uint16_t port,int threadnum):threadnum_(threadnum)
{
	mainloop_ = new EventLoop;
	mainloop_->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));
	
	acceptor_ = new Acceptor(mainloop_,ip,port);
	acceptor_ -> setnewconnectioncb(std::bind(&TcpServer::newconnection,this,std::placeholders::_1));

	threadpool_ = new ThreadPool(threadnum_);	// 创建线程
	
	// 创建从事件循环
	for(int ii=0;ii<threadnum;ii++)
	{
		subloops_.push_back(new EventLoop);	// 创建并存入subloops_中
		subloops_[ii]->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));
		threadpool_->addtask(std::bind(&EventLoop::run,subloops_[ii]));
	}
}

TcpServer::~TcpServer()
{
	delete acceptor_;
	delete mainloop_;
	for(auto& con:conns_)
	{
		delete con.second;
	}
}

void TcpServer::start()
{
	mainloop_->run();
}

void TcpServer::newconnection(Socket* clientsock)
{
	// 改为从事件循环，那该 *conn 将运行在从事件循环里
	// Connection *conn = new Connection(mainloop_,clientsock);
	int idx = clientsock->fd() % threadnum_;	// 一个算法，为了得到一对一的 i
	Connection *conn = new Connection(subloops_[idx],clientsock);			

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
