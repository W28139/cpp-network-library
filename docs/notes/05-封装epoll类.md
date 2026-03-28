## 封装epoll类

新增：`Epoll.h`和`Epoll.cpp`

作用：

封装了`epoll_create` 、`epoll_ctl` 、`epoll_wait`



* `Epoll.h`

```cpp
#pragma once
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/epoll.h>
#include<vector>
#include<unistd.h>

class Epoll
{
private:
    static const int MaxEvents = 100;  // epoll_wait()返回事件数组的大小
    int epollfd_ = -1;               // epoll句柄，在构造函数中创造
    epoll_event events_[MaxEvents];  // 存放epoll_wait返回时间的数组，在构造函数中分配内存
public:
    Epoll();                         // 构造函数，创建epollfd
    ~Epoll();                        // 析构函数，关闭epollfd

    // 把fd和他需要监视的事件添加到红黑树上
    void addfd(int fd,uint32_t op);
    // 实现epoll_wait()等待事件发生，把发生的事件返回至该vector容器内
    std::vector<epoll_event>loop(int timeout = -1);
};
```

* Epoll.cpp

```cpp
#include "Epoll.h"

Epoll::Epoll()
{
    epollfd_ = epoll_create(100);
    if(epollfd_==-1){
        printf("epoll_create () failed %d\n",errno);
        exit(-1);
    }
}

Epoll::~Epoll()
{
    close(epollfd_);
}

void Epoll::addfd(int fd, uint32_t op)
{
    // 创建epoll事件，并添加对应信息
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = op;
    // 把 event 事件添加至 epollfd 内
    int res = epoll_ctl(epollfd_,EPOLL_CTL_ADD,fd,&ev);
    if(res==-1)
    {
        printf("epoll_ctl() failed %d.\n",errno);
        exit(-1);
    }
}

std::vector<epoll_event>Epoll::loop(int timeout)
{
    // 这个数组是用来收集的，内核返回到events_里，我们遍历收集到evs内
    std::vector<epoll_event>evs;
    bzero(events_,sizeof(events_));
    int infds = epoll_wait(epollfd_,events_,MaxEvents ,timeout);
    if(infds<0)
    {
        perror("epoll_wait() failed");
        exit(-1);
    }
    if(infds==0)
    {
        printf("epoll_wait() timeout\n");
        return evs;
    }
    for(int i=0;i<infds;i++)
    {
        evs.push_back(events_[i]);
    }
    return evs;
}
```



* tcpepoll.cpp

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
#include"Socket.h"
#include"Epoll.h"

int main(int argc,char *argv[]){
    if(argc!=3){
        printf("usage:./tcpepoll ip port\n");
        printf("example:./tcpepoll 192.168.150.128 5085\n\n");
        return -1;
    }

    Socket serversock(createnonblocking());
    InetAddress serveraddr(argv[1],atoi(argv[2]));
    serversock.setreuseaddr(true);
    serversock.settcpnodelay(true);
    serversock.setreuseport(true);
    serversock.setkeepalive(true);
    serversock.bind(serveraddr);
    serversock.listen();
    // 以下的listenfd要替换为 serversock.fd(),在结构体里取得
    /*
    int epollfd = epoll_create(1);
    
    struct epoll_event ev;      
    ev.data.fd=serversock.fd(); 
    ev.events=EPOLLIN;  
    
    epoll_ctl(epollfd,EPOLL_CTL_ADD,serversock.fd(),&ev);

    epoll_event evs[1024];
    */
    Epoll ep;
    ep.addfd(serversock.fd(),EPOLLIN);
    std::vector<epoll_event> evs;

    while(true){
        /*
        int infds = epoll_wait(epollfd,evs,1024,-1);
        if(infds<0){
            perror("infds");
            exit(-1);
        }
        if(infds==0){
            perror("infds");
            continue;
        }
        */
        evs = ep.loop();

        //for(int i=0;i<infds;i++){
        for(auto &ev:evs)
        {
            if(ev.events & EPOLLRDHUP){
                printf("client(eventfd=%d) disconnected.\n",ev.data.fd);
                close(ev.data.fd);
            }
            else if(ev.events & (EPOLLIN|EPOLLPRI)){
                if(ev.data.fd==serversock.fd()){
                    InetAddress clientaddr;
                    Socket *clientsock = new Socket(serversock.accept(clientaddr));

                    printf("accept client(fd=%d,ip=%s,port=%d) ok.\n",clientsock->fd(),clientaddr.ip(),clientaddr.port());

                    // ev.data.fd = clientsock->fd();
                    // ev.events = EPOLLIN|EPOLLET;
                    // epoll_ctl(epollfd,EPOLL_CTL_ADD,clientsock->fd(),&ev);
                    ep.addfd(clientsock->fd(),(EPOLLIN|EPOLLET));
                }
                else{
                    char recv_buf[1024];
                    while(true){
                        bzero(&recv_buf,sizeof(recv_buf));
                        ssize_t nread = read(ev.data.fd,recv_buf,sizeof(recv_buf));
                        if(nread>0){
                            printf("recv(eventfd=%d):%s\n",ev.data.fd,recv_buf);
                            write(ev.data.fd, recv_buf, nread);
                        }
                        else if(nread == -1 && errno == EINTR){
                            continue;
                        }
                        else if(nread == -1 &&((errno == EAGAIN)||(errno == EWOULDBLOCK))){
                            break;
                        }
                        else if(nread == 0){
                            printf("client(eventfd=%d) disconncted.\n",ev.data.fd);
                            close(ev.data.fd);
                            break;
                        }
                    }
                }
            }
            else if(ev.events & EPOLLOUT){
            }
            else{
                printf("client(eventfd=%d) error.\n",ev.data.fd);
                close(ev.data.fd);
            }
        }
    }
    return 0;
}
```

