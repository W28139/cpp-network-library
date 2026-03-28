#include"Socket.h"

int createnonblocking()
{
	int listenfd = socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,IPPROTO_TCP);
	if(listenfd<0)
	{
		perror("socket");
		return -1;
	}
	return listenfd;
}

Socket::Socket(int fd):fd_(fd){}

Socket::~Socket()
{
	::close(fd_);
}

int Socket::fd() const
{
	return fd_;
}

void Socket::settcpnodelay(bool on){
        int optval = on?1:0;
        ::setsockopt(fd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof(optval));
}
void Socket::setreuseaddr(bool on){
        int optval = on?1:0;
        ::setsockopt(fd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
}
void Socket::setreuseport(bool on){
        int optval = on?1:0;
        ::setsockopt(fd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof(optval));
}
void Socket::setkeepalive(bool on){
        int optval = on?1:0;
        ::setsockopt(fd_,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof(optval));
}

void Socket::bind(const InetAddress& serveraddr)
{
	int ret = ::bind(fd_,serveraddr.addr(),sizeof(sockaddr));
	ip_ = serveraddr.ip();
	port_ = serveraddr.port();
	if(ret<0)
	{
		perror("bind");
		close(fd_);
		exit(-1);
	}
}


void Socket::listen(int nn)
{
	int ret = ::listen(fd_,nn);
	if(ret<0)
	{
		perror("listen");
		close(fd_);
		exit(-1);
	}
}

int Socket::accept(InetAddress& clientaddr)
{
	sockaddr_in peersock;
	socklen_t len = sizeof(peersock);
	int clientfd = accept4(fd_,(sockaddr*)&peersock,&len,SOCK_NONBLOCK);

	clientaddr.setaddr(peersock);
	ip_ = clientaddr.ip();
	port_ = clientaddr.port();

	return clientfd;
}

std::string Socket::ip() const
{
	return ip_;
}

uint16_t Socket::port() const
{
	return port_;
}
		



