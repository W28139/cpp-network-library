# 多线程主从Reactor模型

* 单线程的Reactor模型不能发挥多核CPU性能
* 运行多个事件循环，主事件循环运行在主线程中，从事件循环运行在线程池中
* 主线程负责创建客户端连接，然后把conn分配给线程池



1. 因为事件循环开始分主从，因此调整一下代码

​	在`TcpServer`类中，主事件改用`EventLoop *mainloop_`表示（堆内存）

​	（在构造函数时，new出来，另外进行delete）

2. 增加从事件循环

由于有多个从事件，因此创建vector<EventLoop*>

3. 增加成员变量：线程池对象和线程池大小(从实现循环的次数)

![](pc/87.png)

4. 补充`TcpServer.cpp`

以上线程池和从事件循环已经创建好，那如何在线程池中运行事件循环呢？

* 把事件循环的run函数作为任务添加给线程池即可

![](pc/88.png)

这里容易发现，在`bind`函数`run`的时候，传了一个参数——对应从事件的地址，但`run()`是无参数的，为什么？

因为`run()`是非静态成员变量，运行它的时候，要么默认传参`this`，要么就把对应对象的地址传进去，代表是：

`subloops_[ii]->run();`

为什么要在 TcpServer 的构造函数中，把所有的 SubLoop 提前创建好并塞进线程池跑起来？

- **构造函数里传进去的（全传）**：是**事件循环（EventLoop）**引擎。必须提前在线程池里跑起来，进入 epoll_wait 监听状态。
- **外界来一个传一个的（后传）**：是**客户端连接（Connection）**。等客户端真的连上来了，再把客户端分配给那些已经跑起来的 EventLoop 去管理。



5. 此时打开服务器，日志显示：

![](pc/90.png)

运行后观察线程:
![](pc/89.png)

进程号和主线程是 278558

而子线程有3个，分别是：278559 - 278601



6. 修改日志，在处理新连接和处理客户端请求报文的时候，都把自己的线程ID打印出来

![](pc/91.png)

易发现，客户端的连接和报文的处理，都是在主进程中，没有在线程池中进行，因此，线程池运行的事件循环，是空的事件循环，因为还没有把connection对象分配给线程池（从事件循环）

因此修改分配问题：
![](pc/92.png)

这时候，已经把报文处理部分放进了线程池里。



但新建客户端连接，依然在主线程进行，原因：

`TcpServer::newconnection()`里执行的新连接，创建的`Connection *conn = new Connection()`

注意，上一段处理，只是修改的这一段代码的传参，因此只是将修改报文内容放进了线程池里，而这个函数它本身的构造，是被回调调用的，如下：

`TcpServer::newconnection() ->Acceptor() -> EchoServer::HandleNewConnection() `

在创建`Acceptor()`时，传入的是`mainloop`

 而在这个函数中，我们打印了线程ID，因此打印的是主线程

![](pc/93.png)

自己推吧，也好推，反复回调、调用，最终会到上面的代码





## 逻辑

### 1. 之前的状态（发现问题）

- **现象**：线程池虽然创建了，也跑起了多个 EventLoop（从事件循环），但它们都是在空转。所有的连接建立（Accept）和数据收发（Read/Write/业务处理）全都在主线程里扎堆执行。
- **原因**：因为你虽然把从事件循环 subloops_ 丢进了线程池，但是**并没有把新产生的客户端连接（Connection 对象）挂载到这些 subloops_ 上**。所有的 Connection 依然默认绑在主线程的 mainloop_ 上。

### 2. 你的修改操作（解决报文处理问题）

- **动作**：你修改了分配逻辑（对应你提到的图92）。在 TcpServer::newconnection() 创建 Connection *conn = new Connection(...) 的时候，你改变了传参，**把线程池里的某一个 subloop（通常是通过轮询/哈希等算法选出一个）传给了这个新建的 Connection**。
- **结果**：这个修改非常成功！新建的 Connection 对象的套接字（Socket）被注册到了所分配的 subloop 的 epoll 中。从此，这个连接后续的“收报文、业务计算、发报文”触发的事件，全都被底层的线程池接管了。

### 3. 你的深入思考（为什么“建立连接”依然在主线程？）

你发现虽然报文处理进了线程池，但打印日志时，发现 TcpServer::newconnection() 这个函数本身的执行依然在主线程。你非常精准地定位到了原因：

- **核心原因在于 Acceptor 的归属**。

- 回顾你之前 TcpServer 的构造函数代码：

  ```C++
  acceptor_ = new Acceptor(mainloop_, ip, port); // 关键在这里！
  acceptor_ -> setnewconnectioncb(std::bind(&TcpServer::newconnection, this, ...));
  ```

- **逻辑链条梳理**：

  1. `Acceptor `负责监听服务端的 listen 端口（监听有没有新客户来）。
  2. 因为 `Acceptor `在创建时绑定的是 `mainloop_`（主线程的事件循环），所以 listen 端口的 EPOLLIN 事件是被主线程的 `epoll_wait `捕获的。
  3. 主线程捕获到事件后，执行 `Acceptor `的回调，进而触发` TcpServer::newconnection()`。
  4. 接着触发了你业务层的 `EchoServer::HandleNewConnection()`。
  5. 因为这一条执行链全是主线程顺藤摸瓜调用下来的，所以在这个过程中打印线程 ID，必然是主线程的 ID。