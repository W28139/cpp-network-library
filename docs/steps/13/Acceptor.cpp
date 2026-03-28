#include"Acceptor.h"
#include"Connection.h"
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

	acceptchannel_->setreadcallback(std::bind(&Acceptor::newconnection,this));
	acceptchannel_->enablereading();
}

Acceptor::~Acceptor()
{
	delete serversock_;
	delete acceptchannel_;
}

void Acceptor::newconnection()
{
	InetAddress clientaddr;
	Socket* clientsock = new Socket(serversock_->accept(clientaddr));

	printf("accept client(fd=%d,ip=%s,port=%d) ok.\n",
		clientsock->fd(),clientaddr.ip(),clientaddr.port());

	Connection *conn = new Connection(loop_,clientsock);			
}
