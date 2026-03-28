#include<signal.h>
#include"EchoServer.h"

EchoServer *echoserver;


void Stop(int sig)
{
	printf("sig=%d\n",sig);
	// 调用EchoServer::Stop()停止服务
	echoserver->Stop();
	printf("echoserver已停止。\n");
	delete echoserver;
	exit(0);
}

int main(int argc,char *argv[])
{
	if(argc!=3)
	{
		printf("nonono");
		return -1;
	}

	signal(SIGTERM,Stop);	// 信号15，系统kill或killall命令默认发送的信号
	signal(SIGINT,Stop);	// 信号2，按Ctrl + C 发送
	
	echoserver = new EchoServer(argv[1],atoi(argv[2]),4,16);
	echoserver->Start();
	
	return 0;
}















































