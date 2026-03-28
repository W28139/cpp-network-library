#include"EchoServer.h"

int main(int argc,char *argv[])
{
	if(argc!=3)
	{
		printf("nonono");
		return -1;
	}
	
	EchoServer echoserver(argv[1],atoi(argv[2]));
	echoserver.Start();
	
	return 0;
}















































