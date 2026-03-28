#pragma once
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string>

class InetAddress
{
private:
	sockaddr_in addr_;
public:
	InetAddress();
	InetAddress(const std::string &ip,const uint16_t port);
	InetAddress(const sockaddr_in addr);
	~InetAddress();

	const char* ip() const;
	uint16_t port() const;
	
	const struct sockaddr *addr() const;

	void setaddr(sockaddr_in addr);
};
