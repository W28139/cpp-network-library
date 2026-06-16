#include"my_muduo/InetAddress.h"

InetAddress::InetAddress(){}

InetAddress::InetAddress(const std::string &ip,const uint16_t port)
{
	addr_.sin_family = AF_INET;
	inet_pton(AF_INET,ip.c_str(),&addr_.sin_addr);
	addr_.sin_port = htons(port);
}

InetAddress::InetAddress(const sockaddr_in addr):addr_(addr){}

InetAddress::~InetAddress(){}

const char* InetAddress::ip() const
{
	return inet_ntoa(addr_.sin_addr);
}

uint16_t InetAddress::port() const
{
	return ntohs(addr_.sin_port);
}

const struct sockaddr* InetAddress::addr() const
{
	return (struct sockaddr*)&addr_;
}

void InetAddress::setaddr(sockaddr_in clientaddr)
{
	addr_ = clientaddr;
}
