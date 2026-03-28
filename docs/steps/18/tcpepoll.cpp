#include"TcpServer.h"

int main(int argc,char *argv[])
{
	if(argc!=3)
	{
		printf("nonono");
		return -1;
	}
	
	TcpServer tcpserver(argv[1],atoi(argv[2]));

	tcpserver.start();
	
	return 0;
}















































