## 分析server端的代码

0. `exit(-1)`与`return -1`的区别：
   * `return -1；`指结束当前函数代码块，将结果返回给该函数的调用者
   * `exit(-1)`是指结束整个进程

1. setsockopt里的参数表示啥？为什么要 `static_cast<socklen_t>`？
   * socketfd，层级，选项的值，选项的长度
   * `static_cast<socklen_t>(sizeof opt)`其中，`opt`是`int`类型，`sizeof opt`返回的是`size_t`类型，而`static_cast<socklen_t>`是把 `size_t` 类型转换成 `socklen_t `类型

2. `sockaddr`与`sockaddr_in`的区别与转换

3. epoll 使用的标准三步

   * 创建 epoll 实例： `epoll_create`

   * 注册要监听的 fd： `struct epoll_event ec`设置监听对象与监听内容，设置`epoll_ctl`

   * 准备接收触发事件 ：`epoll_event evs[1024]`  ，`epoll_wait`接收“触发事件”

     然后不断等待查询即可

4. `epoll_create()`有什么作用？

   * 在内核中创建一个 epoll 实例，并返回一个文件描述符，用来管理一整套“事件监控系统”。

5. 结构体`epoll_event`是什么？

   * 用来描述“监听什么事件”以及“事件发生后返回什么数据”的结构体。

   * ```cpp
     struct epoll_event {
         uint32_t events;   // 监听的事件类型
         epoll_data_t data; // 用户数据
     };
     ```

   * 其中，events有：

   * | 宏       | 含义     |
     | -------- | -------- |
     | EPOLLIN  | 可读     |
     | EPOLLOUT | 可写     |
     | EPOLLERR | 出错     |
     | EPOLLHUP | 断开     |
     | EPOLLET  | 边缘触发 |

   * 结构体`epoll_data `含成员变量 `int fd`，代表对应事件的文件描述符

6. `epoll_ctl()`的含义？作用？

   *  是管理监控名单的函数

   * ```cpp
     int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
     // 作用：向 epoll 实例中添加、修改或删除要监听的文件描述符。
     // epfd  是 epoll_create() 返回的 epollfd
     // op — 操作类型 EPOLL_CTL_ADD(添加监听) EPOLL_CTL_MOD(修改监听) EPOLL_CTL_DEL(删除监听)
     // fd 指要操作的文件描述符，比如：listenfd
     // event，指向 struct epoll_event，只有 ADD 和 MOD 需要，DEL 时可以传 NULL。
     ```

   * 当调用`epoll_ctl`时，内核：
     * 把 fd 插入 epoll 的红黑树，以后开始监听它
     * 记录监听规则

7. `epoll_wait()`的作用：

   * ```cpp
     int epoll_wait(int epfd,
                    struct epoll_event *events,
                    int maxevents,
                    int timeout);
     // 1️⃣ epfd：epoll_create() 返回的 epollfd
     // 2️⃣ events：这是一个“输出数组”，当事件发生时，内核会把触发的事件写到这里。
     // 3️⃣ maxevents：最大返回事件数量，一般是events的大小
     // 4️⃣ timeout：等待时间，-1(永久阻塞)，0(立即返回)，>0(指定毫秒)
     ```

   * 等待被监听的文件描述符发生事件，并返回“已就绪事件”

8. `atoi()`作用？
   * 把字符串强转为整数



```cpp
#include<stdio.h>     // 标准输入输出
#include<unistd.h>    // 基础POSIX接口 close() read() write()
#include<stdlib.h>    // atio() exit()接口
#include<string.h>    // memset() bzero()
#include<errno.h>     
// errno 全局错误变量,配合：
// EINTR  
// EAGAIN
// EWOULDBLOCK
#include<sys/socket.h>  // 套接字核心接口
#include<sys/types.h>   // 套接字核心接口
#include<arpa/inet.h>   // IP 地址转换
#include<sys/fcntl.h>   // 文件控制
#include<sys/epoll.h>   // epoll 多路复用核心
#include<netinet/tcp.h> // TCP 选项

// 设置传入的fd为非阻塞IO
// 让accept read write都不会阻塞
void setnonblocking(int fd){
	fcntl(fd,F_SETFL,fcntl(fd,F_GETFL)|O_NONBLOCK);
}

int main(int argc,char *argv[]){
	// argv[0]	"./tcpepoll"
	// argv[1]	"127.0.0.1"
	// argv[2]	"6789"
	if(argc!=3){
		printf("usage:./tcpepoll ip port\n");
		printf("example:./tcpepoll 192.168.150.128 5085\n\n");
		return -1;
	}

	// 创建监听服务器
	// 第三个参数无所谓
	int listenfd = socket(AF_INET,SOCK_STREAM,0);
	if(listenfd < 0 ){
		perror("socket() failed");
		return -1;
	}
	// 设置listenfd的属性
	int opt = 1;
	// 设置socket选项
	// setsocketopt(fd,level,option,value,length);
	//层级在IPPROTO_TCP，选项是TCP_NODELAY,避免：服务器关闭后会有2*TIME_WAIT的等待时间，使得服务器可以立即重启
	setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,static_cast<socklen_t>(sizeof opt));
	// 选项是SOREUSEPORT，可以复用端口,允许多个进程/线程绑定同一个 IP + PORT
	setsockopt(listenfd,SOL_SOCKET,SO_REUSEPORT,&opt,static_cast<socklen_t>(sizeof opt));
	// 层级在IPPROTO_TCP，选项是TCP_NODELAY，作用是禁用Nagle，不希望延迟。
	setsockopt(listenfd,IPPROTO_TCP,TCP_NODELAY,&opt,static_cast<socklen_t>(sizeof opt));
	// 选项是SO_KEEPALIVE ，在长时间无数据传输时自动探测对端是否存活，及时发现并清理“死连接”。
	setsockopt(listenfd,SOL_SOCKET,SO_KEEPALIVE,&opt,static_cast<socklen_t>(sizeof opt));
		
	// 把服务端listened设置为非阻塞
	setnonblocking(listenfd);

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(atoi(argv[2])); // argv[]是char*类型，应该强转为int
	inet_pton(AF_INET,argv[1],&serveraddr.sin_addr.s_addr);

	if(bind(listenfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr))<0){
		perror("bind");
		close(listenfd);
		return -1;
	}

	if(listen(listenfd,128)!=0){
		perror("listenfd");
		close(listenfd);
		return -1;
	}

	// 创建epoll句柄(理解为创建一个红黑树)
	int epollfd = epoll_create(1);

    // epoll_event理解为叶子
	struct epoll_event ev;      // 声明事件的数据结构 (监控登记表)
	ev.data.fd=listenfd; // 确定“监控对象”
	ev.events=EPOLLIN;   // 确定“监控内容”
	// 理解为把做好的叶子挂在红黑树上，叶子根用listenfd表示，内容用ev表示
	epoll_ctl(epollfd,EPOLL_CTL_ADD,listenfd,&ev);

	epoll_event evs[1024];

	while(true){
        // 等待红黑树epollfd上的叶子有相应，所有有响应的返回到evs里
		int infds = epoll_wait(epollfd,evs,1024,-1);
		// 返回失败
		if(infds<0){
			perror("infds");
			return -1;
		}
		// 超时
		if(infds==0){
			continue;
		}

		for(int i=0;i<infds;i++){
			if(evs[i].data.fd==listenfd){
				struct sockaddr_in clientaddr;
				socklen_t len = sizeof(clientaddr);
				int clientfd = accept(listenfd,(struct sockaddr*)&clientaddr,&len);
				setnonblocking(clientfd);

				printf("accept client(fd=%d,ip=%s,port=%d) ok.\n",
					clientfd,inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
				ev.data.fd = clientfd;
				ev.events = EPOLLIN|EPOLLET;
				epoll_ctl(epollfd,EPOLL_CTL_ADD,clientfd,&ev);
			}
			else{   // 有事件发生
				if(evs[i].events & EPOLLRDHUP){   // 对方已关闭
					printf("client(eventfd=%d) disconnected.\n",evs[i].data.fd);
					close(evs[i].data.fd);
				}
				else if(evs[i].events & (EPOLLIN|EPOLLPRI)){ // 接收缓冲区中有数据可读
					char recv_buf[1024];
					while(true){
						bzero(&recv_buf,sizeof(recv_buf));
						ssize_t nread = read(evs[i].data.fd,recv_buf,sizeof(recv_buf));
						if(nread>0){
							// 把接收到的报文打印出来，然后原封不动发回去
							printf("recv(eventfd=%d):%s\n",evs[i].data.fd,recv_buf);
							write(evs[i].data.fd, recv_buf, nread);
						}
						// 读取数据的时候被信号中断，继续读取
						else if(nread == -1 && errno == EINTR){
							continue;
						}
						// 全部数据读取完毕
						else if(nread == -1 &&((errno == EAGAIN)||(errno == EWOULDBLOCK))){
							break;
						}
						// 客户端断开连接
						else if(nread == 0){
							printf("client(eventfd=%d) disconncted.\n",evs[i].data.fd);
							close(evs[i].data.fd);
							break;
						}
					}
				}
				// 有数据需要写
				else if(evs[i].events & EPOLLOUT){

				}
				// 其它事件，视为错误  
				else{
					printf("client(eventfd=%d) error.\n",evs[i].data.fd);
					close(evs[i].data.fd);
				}
			}
		}
	}
	return 0;
}
```



| 错误码      | 含义               | 出现场景               |
| ----------- | ------------------ | ---------------------- |
| EINTR       | 系统调用被信号打断 | read accept epoll_wait |
| EAGAIN      | 资源暂时不可用     | 非阻塞IO               |
| EWOULDBLOCK | 操作会阻塞         | 非阻塞IO               |
