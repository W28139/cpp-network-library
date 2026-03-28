
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<time.h>

int main(int argc,char *argv[]){
	if(argc!=3){
		printf("usage:./client ip prot\n");
		printf("example:./client 192.168.150.128 2028\n\n");
		return -1;
	}
	
	int sockfd;
	struct sockaddr_in serveraddr;
	char buf[1024];
	if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0){
		printf("socket() failed.\n");
		return -1;
	}
	memset(&serveraddr,0,sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET,argv[1],&serveraddr.sin_addr);

	if(connect(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr))!=0){
		printf("connect(%s:%s)failed.\n",argv[1],argv[2]);
		close(sockfd);
		return -1;
	}

	printf("connect ok\n");

	for(int i=0;i<2000;i++){
		memset(buf,0,sizeof(buf));
		printf("please input:");
		scanf("%s",buf);
		
		if(send(sockfd,buf,strlen(buf),0)<=0){
			printf("read() failed.\n");
			close(sockfd);
			return -1;
		}
		
		memset(buf,0,sizeof(buf));
		
		if(recv(sockfd,buf,sizeof(buf),0)<=0){
			printf("read() failed.\n");
			close(sockfd);
			return -1;
		}

		printf("recv:%s\n",buf);
	}
	return 0;
}




































































