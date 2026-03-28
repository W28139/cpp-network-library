



### `Epoll`的高级用法（加入Channel.h）

在哪里加呢？

```cpp
// epoll_ctl把events写入红黑树时，作为载体
// epoll_wait返回中，该结构体记录发生的事件

struct epoll_event
{
	uint32_t events;
    epoll_data_t data;    // 是共同体，可以携带数据
};

union epoll_data
{
    // 任意指针，可以指向任意内存，可以指向对象，携带更多数据，比只携带fd更方便
    // (自己写一个类，指针指向它，可以携带更多成员函数和成员变量)
    void *ptr; 

    int fd;  // 监听fd,或客户端连接的fd
    uint32_t u32;  // 不用
    uint32_t u64;  // 不用
}
```



在`Reactor`模型中，使用的是`void *ptr`我们让该类指向对象`Channel`

如：

```cpp
class Channel
{
private:
        int fd_;
        bool islisten_ = false;
        // 更多成员变量

public:
        Channel(int fd,bool islisten = false):fd_(fd),islisten_(islisten){}
        int fd(){return fd_;}
        bool islisten(){return islisten_;}
        // 更多的成员函数
};
```



## 添加channel类的过程



1. 修改`Epoll`类里的`addfd()`成员函数，改为 `updatechannel`,其实本质就是把原来 将`epoll_event`挂树上，等效为把`channel`挂树上，因为`channel`的信息更多。
   （虽然`channel`属于`epoll_event.ptr`里的内容，但它可以自己设置，它甚至可以把整个`epoll_event`所有信息都添加进去，还有其他信息）

   

2. 对于成员函数`enablereading()`

之前是把 `epoll_event`单个事件挂到`epoll`红黑树上，但是有了`channel`的话，每个`channel`上会包含该连接的全部信息(自己创建类的成员变量)，因此现在是修改为把`channel`挂到红黑树上，每个`channel`都有标志，显示自己在不在红黑树，还有自己的信息，比如每个`channel`对应的是一个`epoll_event`，它对应的`fd_`,它挂在的`Epoll`的文件描述符，事件`fd_`需要监视的事件，和已发生的事件等信息都在`channel`上

这里面有个特殊一些的成员函数就是`enablereading()`

![](pc/16.png)

![](pc/17.png)

它所被使用的场地为`tcpepoll.cpp`里：

![](pc/20.png)

其实就是把服务器`listenfd`挂到红黑树上，这就是它的作用，它是最特殊的，其他需要挂树上都是会收到事件信号，他需要自己亲自先挂上

当然，除了把自己挂树上，当有新client连接的时候，也需要构建一个新的`channel`，然后挂树上(这个相当于是收到事件信号后的操作)，这里与前面的区别是，新连接要加边缘触发，即要使用`Channel::useet()`成员函数

![](pc/22.png)

3. 把`Epoll`类里的成员函数

`std::vector<epoll_event>loop(int timeout=-1);`
改为
`std::vector<Channel*>loop(int timeout=-1);`

原因同上

![](pc/18.png)

![](pc/19.png)

4. 开始修改对应的`tcpepoll.cpp`即可，比如上面`2`与`3`对应的地方，整体的`epoll_event`改为`channel`的形式，其中有一处判断，有两个方式，体现的依旧是`channel`与`fd`的一对一关系

![](pc/21.png)



5. 进一步封装`Channel`类

由于服务端`tcpepoll.cpp`中，`ep.loop()`返回的时候，处理代码非常繁琐，且只于`channels`相关，因此将他们封装起来，即下面部分：
![](pc/24.png)

因此，添加成员函数`Channel::handleevent()`,把那些所有代码(一直到return 0；前面)

都放入`Channel::handleevent()`里，然后调整格式，由于这是`Channel`类内，可以直接取到成员变量，因此部分代码可以调整，比如`ch->fd()`变为`fd_`即可

在把这一段代码拷贝走的时候，会有很多问题，比如`channel`里缺少头文件，那就需要

`#include"InetAddress.h"`等等，如果发现需要某些外界参数，那就加一个传参，如`Socket *serversock`,，这都是之后发现并改正



6. 对于刚封装的`handleevent()`内，有两个事件的处理流程不同，如下：

![](pc/25.png)

而当前`channel`类还未区分是哪个`fd`的成员变量，因此增加成员变量

`bool islistenfd_ = false; `作为判断依据

同时修改构造函数，服务端函数等

增加判断方式即：

![](pc/26.png)







* `Channel.h`

```cpp
#pragma once
#include<sys/epoll.h>
#include"Epoll.h"

class Epoll;

class Channel
{
private:
	// channel拥有的fd,Channel和fd是一对一关系
	int fd_;
	// channel对应的红黑树，channel和Epoll是多对一关系，一个channel只能对应一个Epoll
	Epoll *ep_ = nullptr;
	// channel是否已添加到红黑树上,如果未添加，调用epoll_ctl()的时候用EPOLL_CTL_ADD,否则用EPOLL_CTL_MOD
	bool inepoll_ = false;
	// fd需要监视的事件。listenfd和clientfd需要监视EPOLLIN,clientfd可能需要监视EPOLLOUT
	uint32_t events_ = 0;
	// fd_已发生事件
	uint32_t revents_ = 0;
public:
	Channel(Epoll *ep,int fd);   // 构造函数
	~Channel();                  // 析构函数

	int fd();                    // 返回fd_成员
	void useet();                // 采用边缘触发
	void enablereading();        // 让epoll_wait()监听fd_的读事件(不是很懂，看着像把自己挂树上,好像就是把自己加进去)
	void setinepoll();           // 把inepoll_成员设置为true
	void setrevents(uint32_t ev);// 设置revents_成员的值为 ev
	bool inepoll();              // 返回inpoll_成员
	uint32_t events();           // 返回events_成员
	uint32_t revents();          // 返回revents_成员
};
```

* `Channel.cpp`

```cpp
#include"Channel.h"

Channel::Channel(Epoll* ep,int fd):ep_(ep),fd_(fd)
{

}

Channel::~Channel()
{
	// 不能销毁ep_,也不能关闭fd,因为这是从外面传进来的，不是自己创建的，Channel只是使用它们
}

int Channel::fd()
{
	return fd_;
}

void Channel::useet()
{
	events_ = events_|EPOLLET;
}

void Channel::enablereading()
{
	events_ = events_|EPOLLIN; // 修改事件，注意设定方式，要用|
	ep_->updatechannel(this);
}
void Channel::setinepoll()
{
	inepoll_ = true;
}

void Channel::setrevents(uint32_t ev)
{
	revents_ = ev;
}

bool Channel::inepoll()
{
	return inepoll_;

}

uint32_t Channel::events()
{
	return events_;
}

uint32_t Channel::revents()
{
	return revents_;
}
```



`Epoll.h`新增函数，修改之前的`addfd`，改为把整个`channel`添加到红黑树上

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
  
  // 添加 Channel头文件
  #include"Channel.h"
  // 前置声明
  class Channel;
  
  class Epoll
  {
  private:
          static const int MaxEvents = 100;
          int epollfd_ = -1;
          epoll_event events_[MaxEvents];
  public:
          Epoll();
          ~Epoll();
  
          // void addfd(int fd,uint32_t op);
          // 改为把Channel类添加到红黑树上,方式epoll_ctl()
          void updatechannel(Channel *ch);  // 把channel添加/更新到红黑树上，channel有fd和需要监听事件
  
          // std::vector<epoll_event>loop(int timeout = -1);
          // 改为Channel*
          std::vector<Channel*>loop(int timeout = -1);
  };
  ```

  

* `Epoll.cpp`

```cpp
#include "Epoll.h"

Epoll::Epoll()
{
	int epollfd_ = epoll_create(100);
	if(epollfd_==-1){
		printf("epoll_create () failed %d\n",errno);
		exit(-1);
	}
}

Epoll::~Epoll()
{
	close(epollfd_);
}
/*
void Epoll::addfd(int fd, uint32_t op)
{
	epoll_event ev;
	ev.data.fd = fd;
	ev.events = op;
	int res = epoll_ctl(epollfd_,EPOLL_CTL_ADD,fd,&ev);
	if(res==-1)
	{
		printf("epoll_ctl() failed %d.\n",errno);
		exit(-1);
	}
}
*/

void Epoll::updatechannel(Channel *ch)
{
	epoll_event ev;
	ev.data.ptr=ch;
	ev.events = ch->events();

	if(ch->inepoll()) // 如果channel已经在树上，就更新
	{
		int ret = epoll_ctl(epollfd_,EPOLL_CTL_MOD,ch->fd(),&ev);
		if(ret==-1)
		{
			perror("epoll_ctl() failed. \n");
			exit(-1);
		}
	}
	else           // 如果不在树上，就先添加到树上
	{
		int ret = epoll_ctl(epollfd_,EPOLL_CTL_ADD,ch->fd(),&ev);
		if(ret==-1)
		{
			perror("epoll_ctl() failed.\n");
			exit(-1);
		}
		ch->setinepoll(); // 把channel的inepoll——成员设置为true,标志在树上
	}
}
/*
std::vector<epoll_event>Epoll::loop(int timeout)
{
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
*/

std::vector<Channel *>Epoll::loop(int timeout)
{
	std::vector<Channel *>channels;
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
		return channels;
	}
	for(int i=0;i<infds;i++)
	{
		// evs.push_back(events_[i]);
        // 这里先取出有事件发生的ch(相当于fd,因为一一对应)
		Channel *ch = (Channel *)events_[i].data.ptr;  // 取出已发生事件的channel
        // 在对应的ch里记录下所发生的事件，这也就是revents_的作用了
		ch->setrevents(events_[i].events);             // 设置channel的revents_成员
        // 把所有有事件发生的events_(channel)打包返回
		channels.push_back(ch);
	}
	return channels;
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

	Epoll ep;

	// ep.addfd(serversock.fd(),EPOLLIN);  作用: 把fd和它监视的事件添加到红黑树上
	// 但是已经有了Channel类，它包括fd和监视事件，因此可以把它整个添加到红黑树上,方式:epoll_ctl()
	Channel *serverchannel = new Channel(&ep,serversock.fd());
	serverchannel->enablereading();


	while(true){
		std::vector<Channel *> channels = ep.loop();

		for(auto &ch:channels)
		{
			if(ch->revents() & EPOLLRDHUP){
				printf("client(eventfd=%d) disconnected.\n",ch->fd());
				close(ch->fd());
			}
			else if(ch->revents() & (EPOLLIN|EPOLLPRI)){
				if(ch ==serverchannel){
					InetAddress clientaddr;
					Socket *clientsock = new Socket(serversock.accept(clientaddr));

					printf("accept client(fd=%d,ip=%s,port=%d) ok.\n",clientsock->fd(),clientaddr.ip(),clientaddr.port());

					// ep.addfd(clientsock->fd(),(EPOLLIN|EPOLLET));
					// 当新客户端连接时，new一个客户端的channel
					Channel *clientchannel = new Channel(&ep,clientsock->fd());
					serverchannel->enablereading();
					clientchannel->useet(); // 采用边缘触发
				}
				else{
					char recv_buf[1024];
					while(true){
						bzero(&recv_buf,sizeof(recv_buf));
						ssize_t nread = read(ch->fd(),recv_buf,sizeof(recv_buf));
						if(nread>0){
							printf("recv(eventfd=%d):%s\n",ch->fd(),recv_buf);
							write(ch->fd(), recv_buf, nread);
						}
						else if(nread == -1 && errno == EINTR){
							continue;
						}
						else if(nread == -1 &&((errno == EAGAIN)||(errno == EWOULDBLOCK))){
							break;
						}
						else if(nread == 0){
							printf("client(eventfd=%d) disconncted.\n",ch->fd());
							close(ch->fd());
							break;
						}
					}
				}
			}
			else if(ch->revents() & EPOLLOUT){
			}
			else{
				printf("client(eventfd=%d) error.\n",ch->fd());
				close(ch->fd());
			}

		}
	}
	return 0;
}
```



#### 补充完善

观察`tcpepoll.cpp`内容，发现有很多代码只与`channel`相关，因此可以封装进去

新增以下内容：

* 在`channel.h`里，增加函数`handle_event()`  作用：事件处理函数，epoll_wait()返回的时候执行 

* 增加成员变量`islisten`,用于判断是否是`listen`接收到的新连接（来自客户端），在构造`channel`时，自己判断写入`false/true`即可

* `channel.h`

  ```cpp
  #include<sys/epoll.h>
  #include"Epoll.h"
  #include"InetAddress.h"
  #include"Socket.h"
  
  class Epoll;
  
  class Channel
  {
  private:
          int fd_;
          Epoll *ep_ = nullptr;
          bool inepoll_ = false;
          uint32_t events_ = 0;
          uint32_t revents_ = 0;
  
          bool islisten_=false; // listenfd 取值为true，客户端连上来的fd取值为false
  
  public:
          Channel(Epoll *ep,int fd,bool islisten);
          ~Channel();
  
          int fd();
          void useet();
          void enablereading();
          void setinepoll();
          void setrevents(uint32_t ev);
          bool inepoll();
          uint32_t events();
          uint32_t revents();
  
          void handle_event(Socket *serversock);         // 事件处理函数，epoll_wait()返回的时候执行 
  };
  ```
  
  
  
* `channel.cpp`

```cpp
#include"Channel.h"

Channel::Channel(Epoll* ep,int fd,bool listen):ep_(ep),fd_(fd),islisten_(listen)
{

}

Channel::~Channel()
{
// 不能销毁ep_,也不能关闭fd,因为这是从外面传进来的，不是自己创建的，Channel只是使用它们
}

int Channel::fd()
{
        return fd_;
}

void Channel::useet()
{
        events_ = events_|EPOLLET;
}

void Channel::enablereading()
{
        events_ = events_|EPOLLIN; // 修改事件，注意设定方式，要用|
        ep_->updatechannel(this);
}
void Channel::setinepoll()
{
        inepoll_ = true;
}

void Channel::setrevents(uint32_t ev)
{
        revents_ = ev;
}

bool Channel::inepoll()
{
        return inepoll_;

}

uint32_t Channel::events()
{
        return events_;
}

uint32_t Channel::revents()
{
        return revents_;
}

void Channel::handle_event(Socket *serversock)
{
        if(revents_ & EPOLLRDHUP)
        {
                printf("client(eventfd=%d) disconnected.\n",fd_);
                close(fd_);
        }
        else if(revents_ & (EPOLLIN|EPOLLPRI))
        {
                if(islisten_ == true)
                {
                        InetAddress clientaddr;
                        Socket *clientsock = new Socket(serversock->accept(clientaddr));
                        printf("accept client(fd = %d,ip = %s,port = %d) ok.\n",clientsock->fd(),clientaddr.ip(),clientaddr.port());

                        Channel *clientchannel = new Channel(ep_,clientsock->fd(),false);
                        clientchannel->useet();
                        clientchannel->enablereading();
                }
                else
                {
                        char buffer[1024];
                        while(true)
                        {
                                bzero(&buffer,sizeof(buffer));
                                ssize_t nread = read(fd_,buffer,sizeof(buffer));
                                if(nread>0)
                                {
                                        printf("recv(eventfd=%d):%s\n",fd_,buffer);
                                        send(fd_,buffer,strlen(buffer),0);
                                }
                                else if(nread == -1 && errno == EINTR)
                                {
                                        continue;
                                }
                                else if(nread == -1 && ((errno ==EAGAIN) || ((errno == EWOULDBLOCK))))
                                {
                                        break;
                                }
                                else if(nread == 0 )
                                {
                                        printf("client(evnetfd=%d) disconnected.\n",fd_);
                                        close(fd_);
                                        break;
                                }
                        }
                }
        }
        else if(revents_ & EPOLLOUT)
        {
        }
        else
        {
                printf("client(eventfd=%d error.\n",fd_);
                close(fd_);
        }
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

        Epoll ep;

        Channel *serverchannel = new Channel(&ep,serversock.fd(),true);
        serverchannel->enablereading();


        while(true){
                std::vector<Channel *> channels = ep.loop();

                for(auto &ch:channels)
                {
                        ch->handle_event(&serversock);
                }
        }
        return 0;
}
```



补充一个bug:

如果在一个类里，需要用到另一个类，那就需要前置声明以下，是

`class Channel`而不是`#include"Channel.h"`

![](pc/23.png)



