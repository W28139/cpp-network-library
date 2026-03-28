#include"TcpServer.h"
TcpServer::TcpServer(const std::string &ip,const uint16_t port,int threadnum)
	:threadnum_(threadnum),mainloop_(new EventLoop(true)),acceptor_(mainloop_.get(),ip,port),threadpool_(threadnum,"IO")
{
	// mainloop_ = new EventLoop;
	mainloop_->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));
	
	// acceptor_ = new Acceptor(mainloop_,ip,port);
	acceptor_.setnewconnectioncb(std::bind(&TcpServer::newconnection,this,std::placeholders::_1));

	// threadpool_ = new ThreadPool(threadnum_,"IO");	// 创建线程
	
	// 创建从事件循环
	for(int ii=0;ii<threadnum;ii++)
	{
		subloops_.emplace_back(new EventLoop(false,5,10));	// 创建并存入subloops_中
		subloops_[ii]->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));
		subloops_[ii]->settimercallback(std::bind(&TcpServer::removeconn,this,std::placeholders::_1));
		
		threadpool_.addtask(std::bind(&EventLoop::run,subloops_[ii].get()));
	}
}

TcpServer::~TcpServer()
{
	// delete acceptor_;
	// delete mainloop_;
	// for(auto &aa:subloops)
	// {
	// 	delete aa;
	// }
	// delete threadpool_;
}

void TcpServer::start()
{
	mainloop_->run();
}

void TcpServer::newconnection(std::unique_ptr<Socket> clientsock)
{
	int idx = clientsock->fd() % threadnum_;	// 一个算法，为了得到一对一的 i
	spConnection conn(new Connection(subloops_[idx].get(),std::move(clientsock)));			

	conn->setclosecallback(std::bind(&TcpServer::closeconnection,this,std::placeholders::_1));
	conn->seterrorcallback(std::bind(&TcpServer::errorconnection,this,std::placeholders::_1));
	conn->setonmessagecallback(std::bind(&TcpServer::onmessage,this,std::placeholders::_1,std::placeholders::_2));
	conn->setsendcompletecallback(std::bind(&TcpServer::sendcomplete,this,std::placeholders::_1));

	{
		std::lock_guard<std::mutex>gd(mmutex_);
		conns_[conn->fd()] = conn;
	}
	subloops_[idx]->newconnection(conn);	// 将连接加入到 事件循环的 map 里
	
	// 建立后回调
	if(newconnectioncb_) newconnectioncb_(conn);
}

void TcpServer::closeconnection(spConnection conn)
{
	// 析构前回调
	if(closeconnectioncb_)
	{
		closeconnectioncb_(conn);
	}

	{
		std::lock_guard<std::mutex>gd(mmutex_);
		conns_.erase(conn->fd());
	}
}
void TcpServer::errorconnection(spConnection conn)
{
	if(errorconnectioncb_){
		errorconnectioncb_(conn);
	}

	{
		std::lock_guard<std::mutex>gd(mmutex_);
		conns_.erase(conn->fd());
	}
}

void TcpServer::onmessage(spConnection conn,std::string& message)
{
	if(onmessagecb_) onmessagecb_(conn,message);
}

void TcpServer::sendcomplete(spConnection conn)
{
	if(sendcompletecb_) sendcompletecb_(conn);
}

void TcpServer::epolltimeout(EventLoop *loop)
{
	if(timeoutcb_) timeoutcb_(loop);
}


void TcpServer::setnewconnectioncb(std::function<void(spConnection)>fn)
{
	newconnectioncb_ = fn;
}
void TcpServer::setcloseconnectioncb(std::function<void(spConnection)>fn)
{
	closeconnectioncb_ = fn;
}
void TcpServer::seterrorconnectioncb(std::function<void(spConnection)>fn)
{
	errorconnectioncb_ =fn;
}
void TcpServer::setonmessagecb(std::function<void(spConnection,std::string &message)>fn)
{
	onmessagecb_ = fn;
}
void TcpServer::setsendcompletecb(std::function<void(spConnection)>fn)
{
	sendcompletecb_ = fn;
}
void TcpServer::settimeoutcb(std::function<void(EventLoop*)>fn)
{
	timeoutcb_ = fn;
}

void TcpServer::removeconn(int fd)
{
	std::lock_guard<std::mutex>gd(mmutex_);
	conns_.erase(fd);
}
