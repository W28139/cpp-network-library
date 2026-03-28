#include"Acceptor.h"

Acceptor::Acceptor(EventLoop* loop,const std::string &ip,const uint16_t port):loop_(loop)
{
	serversock_  = new Socket(createnonblocking());

	InetAddress serveraddr(ip,port);
	serversock_->setreuseaddr(true);
	serversock_->settcpnodelay(true);
	serversock_->setreuseport(true);
	serversock_->setkeepalive(true);
	serversock_->bind(serveraddr);
	serversock_->listen();

	acceptchannel_ = new Channel(loop_,serversock_->fd());
	acceptchannel_->setreadcallback(std::bind(&Channel::newconnection,acceptchannel_,serversock_));
	acceptchannel_->enablereading();
}

Acceptor::~Acceptor()
{
	delete serversock_;
	delete acceptchannel_;
}
