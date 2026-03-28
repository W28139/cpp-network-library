
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
  	for (int ii=0;ii<100;ii++)
        {
   	    	 memset(buf,0,sizeof(buf));
       		 sprintf(buf,"这是第%d个超级女生。",ii);

       		 char tmpbuf[1024];    	 // 临时的buffer，报文头部+报文内容。
       		 memset(tmpbuf,0,sizeof(tmpbuf));
       		 int len=strlen(buf);   	 // 计算报文的大小。
       		 memcpy(tmpbuf,&len,4);   	 // 拼接报文头部。
       		 memcpy(tmpbuf+4,buf,len);       // 拼接报文内容。

        	send(sockfd,tmpbuf,len+4,0);     // 把请求报文发送给服务端。
   	 }
        
   	 for (int ii=0;ii<100;ii++)
    {
		int len;	// 存放包头
		recv(sockfd,&len,4,0);	// 先读取一个头到len内
		memset(buf,0,sizeof(buf));  
		recv(sockfd,buf,len,0);  // 读取下面内容到空 buf 内
       	printf("recv:%s\n",buf); 
   	 }
} 
