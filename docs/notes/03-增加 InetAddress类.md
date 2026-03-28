## InetAddress类

1. 核心修改内容如下：

由于地址和协议结构体`sockaddr_in`创建的复杂性，因此单独封装

新增 `InetAddress.h`和`InetAddress.cpp`

在`InetAddress.h`中，写一个class类，命名为InetAddress

（将老版本的`inet_ntoa`转为现代的`inet_ntop`）

注意，这个改东的影响范围是`struct sockaddr_in serveraddr;`的区域，局部优化代码结构



2. 为什么要有俩构造函数？

* 第一个传 IP、port，在InetAddress类里自己创建

* 第二个直接传已创建好的结构体sockaddr_in

原因：第一个地址协议结构体是未知，需要自己创建方便使用、传递给accept函数，但第二个是存在已经生成好了的结构体，来自client端，直接传入即可，无需自己根据IP、port创建





3. 以下有四个文件：

* InetAddress.h (只定义，不写内容)

  ```cpp
  #pragma once // 避免重复引入头文件
  #include<arpa/inet.h>
  #include<netinet/in.h>
  #include<string>
  
  // sock的地址协议类
  class InetAddress
  {
  private:
  		 // 表示地址协议的结构体(类的成员变量命名要加下划线m前后都可)
          sockaddr_in addr_; 
  
  public:
          // (构造函数)如果是监听fd,那用这个构造fd
          InetAddress(const std::string &ip,uint16_t port);
          // (构造函数)如果是客户端连上来的fd,用这个构造函数
          InetAddress(const sockaddr_in addr);
          ~InetAddress();
  
          const char* ip() const; // 返回字符串，表示地址
          uint16_t port() const;  // 返回端口,类型是unsigned int
      	// 返回addr_成员的地址，并转换成为sockaddr
          const struct sockaddr *addr() const;  
  
  };
  ```

* `InetAddress.cpp`,完善`InetAddress.h`里定义的内容

```cpp
#include "InetAddress.h"

// 初始化构造函数，接收IP、port，构造出 sockaddr_in 结构体
InetAddress::InetAddress(const std::string &ip,uint16_t port){
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        inet_pton(AF_INET,ip.c_str(),&addr_.sin_addr);
}
// 初始化构造函数，直接得到sockaddr_in 结构体，因为传入的就是
InetAddress::InetAddress(const sockaddr_in addr):addr_(addr){}

// 析构函数，清理函数
InetAddress::~InetAddress(){}
// 取结构体sockaddr_in里的IP信息，并且进行转化为人看的
const char* InetAddress::ip() const{
        return inet_ntoa(addr_.sin_addr); // 目前这一个是被淘汰的，容易出错且仅适用于IPV4
}
// 取结构体sockaddr_in里的port信息
uint16_t InetAddress::port() const{
        return ntohs(addr_.sin_port);
}
// 转换结构体类型，转为传入接口函数(accept、connect)的类型 sockaddr_in -> sockaddr
const struct sockaddr* InetAddress::addr() const{
        return (sockaddr*)&addr_;
}
```

* `tcpepoll.cpp`    

```cpp
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<sys/fcntl.h>
#include<sys/epoll.h>
#include<netinet/tcp.h>
#include"InetAddress.h"

int main(int argc,char *argv[]){
    if(argc!=3){
        printf("usage:./tcpepoll ip port\n");
        printf("example:./tcpepoll 192.168.150.128 5085\n\n");
        return -1;
    }

    int listenfd = socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,IPPROTO_TCP);
    if(listenfd < 0 ){
        perror("socket() failed");
        return -1;
    }

    int opt = 1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,static_cast<socklen_t>(sizeof opt));
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEPORT,&opt,static_cast<socklen_t>(sizeof opt));
    setsockopt(listenfd,IPPROTO_TCP,TCP_NODELAY,&opt,static_cast<socklen_t>(sizeof opt));
    setsockopt(listenfd,SOL_SOCKET,SO_KEEPALIVE,&opt,static_cast<socklen_t>(sizeof opt));

    // 初始化创建类serveraddr, 这里直接传地址和端口，利用构造函数1可得到结构体addr_
    // 注意，原来serveraddr是结构体，现在是类，== serveraddr.addr()才是结构体
    InetAddress serveraddr(argv[1],atoi(argv[2]));
    // 这里使用InetAddress里的成员函数，直接返回sockaddr类型,大小设置为sockaddr
    // if(bind(listenfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr))<0)
    if(bind(listenfd,serveraddr.addr(),sizeof(sockaddr))<0){
        perror("bind");
        close(listenfd);
        return -1;
    }

    if(listen(listenfd,128)!=0){
        perror("listenfd");
        close(listenfd);
        return -1;
    }

    int epollfd = epoll_create(1);

    struct epoll_event ev;
    ev.data.fd=listenfd;
    ev.events=EPOLLIN;

    epoll_ctl(epollfd,EPOLL_CTL_ADD,listenfd,&ev);

    epoll_event evs[1024];

    while(true){
        int infds = epoll_wait(epollfd,evs,1024,-1);
        if(infds<0){
            perror("infds");
            exit(-1);
        }
        if(infds==0){
            perror("infds");
            continue;
        }

        for(int i=0;i<infds;i++){
            if(evs[i].events & EPOLLRDHUP){
                printf("client(eventfd=%d) disconnected.\n",evs[i].data.fd);
                close(evs[i].data.fd);
            }
            else if(evs[i].events & (EPOLLIN|EPOLLPRI)){
                if(evs[i].data.fd==listenfd){
                    struct sockaddr_in peeraddr;
                    socklen_t len = sizeof(peeraddr);
                    int clientfd = accept4(listenfd,(struct sockaddr*)&peeraddr,&len,SOCK_NONBLOCK);
					// 第二次使用 InetAddress类，使用第二个构造函数，直接传入结构体sockaddr_in
                    InetAddress clientaddr(peeraddr);
                    printf("accept client(fd=%d,ip=%s,port=%d) ok.\n",clientfd,clientaddr.ip(),clientaddr.port());

                    ev.data.fd = clientfd;
                    ev.events = EPOLLIN|EPOLLET;
                    epoll_ctl(epollfd,EPOLL_CTL_ADD,clientfd,&ev);
                }
                else{
                    char recv_buf[1024];
                    while(true){
                        bzero(&recv_buf,sizeof(recv_buf));
                        ssize_t nread = read(evs[i].data.fd,recv_buf,sizeof(recv_buf));
                        if(nread>0){
                            printf("recv(eventfd=%d):%s\n",evs[i].data.fd,recv_buf);
                            write(evs[i].data.fd, recv_buf, nread);
                        }
                        else if(nread == -1 && errno == EINTR){
                            continue;
                        }
                        else if(nread == -1 &&((errno == EAGAIN)||(errno == EWOULDBLOCK))){
                            break;
                        }
                        else if(nread == 0){
                            printf("client(eventfd=%d) disconncted.\n",evs[i].data.fd);
                            close(evs[i].data.fd);
                            break;
                        }
                    }
                }
            }
            else if(evs[i].events & EPOLLOUT){
            }
            else{
                printf("client(eventfd=%d) error.\n",evs[i].data.fd);
                close(evs[i].data.fd);
            }
        }
    }
    return 0;
}
```



* 另外，创建了个`makefile`，只需要输入 make 就会直接编译好

```makefile
all:client tcpepoll

client:client.cpp
        g++ -g -o client client.cpp

tcpepoll:tcpepoll.cpp InetAddress.cpp
        g++ -g -o tcpepoll tcpepoll.cpp InetAddress.cpp

clean:
        rm -f client tcpepoll

```