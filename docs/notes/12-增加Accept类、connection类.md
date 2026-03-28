# 增加Accept类

为啥增加这个？

* Channel（通道）封装了监听fd和客户端连接的fd

* 监听 fd 与客户端连接的 fd 功能是不同的
* 生命周期不同，监听fd全程存在

那如何做？

* 在Channel之上再做一层封装
  * 监听Channel封装为Acceptor类
  * 客户端连接的Channel封装成 Connection类



现在存在的问题是，new出的Channel、Socket 无法销毁

![](pc/9.png)

![](pc/10.png)



我们新加的Acceptor类\Connection类，设置为 TcpServer类的成员变量，它是网络服务类，会有一个Acceptor类对象和无数个Connection类对象

### 为什么不放在 EventLoop（事件循环）里？

**EventLoop 是“引擎”/“CPU”**：它的职责非常纯粹，就是死循环运行 epoll_wait，处理事件。

**TcpServer 是“控制台”**：它负责业务逻辑的组织。它创建 Connection 对象，然后把这个对象“注册”到 EventLoop 中去运行。

- **EventLoop 是跑道**。
- **Connection 是飞机**。
- **TcpServer 是航空公司**。

### 总结它们的工作流程

1. **启动**：TcpServer 启动，初始化内部唯一的 Acceptor，开始在特定端口监听。
2. **连接**：客户端发起连接，Acceptor 感知到，完成 TCP 三次握手。
3. **分发**：Acceptor 拿到新的 Socket，交给 TcpServer 说：“新连接来了”。
4. **创建**：TcpServer 创建一个新的 Connection 对象来接管这个 Socket。
5. **存储**：TcpServer 把这个新 Connection 扔进自己的 Map 或 List 里存起来（这就是那“无数个”对象的由来）。
6. **通信**：以后这个客户端发来的所有数据，都由这个对应的 Connection 对象处理，Acceptor 继续回到门口去等下一个新客人。

TcpServer 是**公司**，Acceptor 是**HR（负责招人/接客）**，Connection 是**干活的员工（负责具体的业务交互）**。HR 只有一个，但员工可以有成千上万个。



### 接下来就是处理代码了

我们先封装`Accept`类，第一步就是将之前`TcpServer`构造函数里的那些代码移到`Accept`里

![](pc/31.png)

先创建基础的`Acceptor`类，其中成员变量和构造函数由上面截图代码来添加，如下：

![](pc/32.png)

确保完全替代掉`TcpServer`构造函数里的内容，把他们完全提取出来，单独成类

其中具体内容为：
![](pc/33.png)

注意，销毁的对象是自己new的，而不能是外界传进来的

到此`Accept`类已经准备完毕，现在开始修改`TcpServer`类

自然是把类加到它的成员变量里，然后再修改构造函数即可

![](pc/34.png)

![](pc/35.png)



# 增加connection类

封装服务端用于通信的`Channel`，找到`connectchannel`创建的位置(`channel.newconnection`内)，然后把他整理到connection类的构造函数内，根据需要判断传参

![](pc/36.png)

需要传参：`EventLoop *loop`和`Socket *clientsock`



在写`Connection`类的构造析构函数时，有一处细节，虽然`clientsock_`是外界传进来的，但是，但是有两个条件，使得它必须在析构函数里销毁：

* 外界传进来时是new的，但是外界没有delete
* 使用完`Connection`类之后，这个对象就没用了，在`newconnection()`的代码块里不再被使用

![](pc/37.png)

因此，虽然是外界传进来的，但是依然是要销毁的，最后修改`newconnection()`里的内容

![](pc/38.png)

很容易发现，这个新`new`的对象依旧没有`delete`，会有内存泄漏，此问题之后再说

