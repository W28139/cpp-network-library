#include"Acceptor.h"
#include"Connection.h"
Acceptor::Acceptor(EventLoop* loop,const std::string &ip,const uint16_t port)
	:loop_(loop),serversock_(createnonblocking()),acceptchannel_(loop_,serversock_.fd())
{
	InetAddress serveraddr(ip,port);
	serversock_.setreuseaddr(true);
	serversock_.settcpnodelay(true);
	serversock_.setreuseport(true);
	serversock_.setkeepalive(true);
	serversock_.bind(serveraddr);
	serversock_.listen();

	acceptchannel_.setreadcallback(std::bind(&Acceptor::newconnection,this));
	acceptchannel_.enablereading();
}

Acceptor::~Acceptor()
{
}

void Acceptor::newconnection()
{
	InetAddress clientaddr;

	// Socket* clientsock = new Socket(serversock_.accept(clientaddr));
	std::unique_ptr<Socket>clientsock(new Socket(serversock_.accept(clientaddr)));

	clientsock->setipport(clientaddr.ip(),clientaddr.port());

	newconnectioncb_(std::move(clientsock));
}

void Acceptor::setnewconnectioncb(std::function<void(std::unique_ptr<Socket>)> fn)
{
	newconnectioncb_ = fn;
}









