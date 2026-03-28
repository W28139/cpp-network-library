### 为什么需要缓冲区buff

给一个场景，如果client向客户端发送1000条报文，每条报文**间隔十万分之一秒发**送与**无间隔发送**时，server收到的报文如下：

![](pc/62.png)

![](pc/63.png)

如上，如果通讯速度很快，就会发生：

* 发生粘包，即多个报文粘在一块
* 发生分包，即一个报文被分为两个报文
* 发生乱码

粘包是指发送方发送的多个独立的应用程序数据包（消息），在接收方读取时被“粘”在了一起，变成了一个连续的数据块。接收方无法直接从数据本身区分哪里是第一个消息的结尾，哪里是第二个消息的开头。

分包（或拆包）是指发送方发送的一个完整的应用程序数据包，在传输或接收过程中被截断了，导致接收方在一次读取操作中，只读到了这个包的一部分（即所谓的“半包”），需要多次读取才能拼凑出一条完整的消息。



### 如何解决？

1. **固定长度：** 规定双方通信的每个消息长度固定（比如一律是 100 字节），如果消息不够长就用空格或 0x00 补齐。接收方每次死板地读满 100 字节就算一个包。（*缺点：浪费带宽*）
2. **特殊分隔符：** 在每个消息的末尾加上特殊的边界字符（例如 HTTP、FTP 协议常用的换行符 \r\n）。接收方不断接收流数据，扫描到特定的分隔符就认为一个完整消息结束。（*缺点：消息正文里绝对不能出现该分隔符，否则需要做复杂的转义*）
3. **消息头 + 消息体：** **最常用、最推荐的做法**。在真正的数据前加一个固定长度的“包头”，包头里包含一个指示“包体长度”的整数。接收方先读取包头，解析出包体应该有多少字节，然后再精确读取对应长度的字节数作为完整消息。





### buffer的必要性

在**非阻塞**的网络服务程序中，**事件循环不会阻塞在recv和send中**，如果数据接收不完整，或者发送缓冲区已填满，都不能等待，所以buffer是必须的

在Reactor模型中，每个**Connection**对象，拥有一个接收**InputBuffer**和**SendBuffer**



# 具体实现：

## 增加`Buffer`类

1. 增加`Buffer`类

![](pc/64.png)



2. 修改`Connection`类，增加接收缓冲区和发送缓冲区

![](pc/65.png)

3. 此时发现，在`Connection`类的成员函数中，没有读取数据的代码，而读取数据的代码在`Channel`

类中，如下：

![](pc/66.png)

这是之前创建`Channel`类的时候，统一放进去的，因为但是还没把`Connection`单独写出来，此时就修改此处，将`Channel`类里处理数据的部分，拿到`Connection`里面，然后修改橙色部分

然后还要修改回调函数，因为`Connection`类里有，因此可以直接调用
(这里的回调是之前`Channel->connection->TcpServer`时写的)，这里只是减少一层

由于`message`所属类发生变化，同样要修改创建回调函数部分：

![](pc/67.png)

这里的回调函数依旧是留给`Channel`类调用

最后，把`Buffer`类的使用加进去，得到：
![](pc/68.png)

上述改动只是增加一个类，并未解决粘包分包的问题，现在开始处理

## 如何使用接收缓冲区inputbuffer？

情景a:
如果采用同步的发送接收方式，即循环中，先发送一个数据包，然后接收，接收后再发的话，那不需要任何其他操作，不会发生粘包和分包，但效率极低

![](pc/69.png)

情景b:

如果修改为以下的代码：

![](pc/70.png)

那就会发生粘包和分包的问题

![](pc/71.png)



### 采用"消息头 + 消息体"的方式解决

在`server`端，循环读取，以每个包为单位进行打印

![](pc/72.png)

在`client`内修改为：
![](pc/73.png)

同样，可以做优化：

![](pc/74.png)



### 优化代码结构

在Reactor模型中，`Connection`是底层类，不承担数据计算工作，因此把它放到`TcpServer`中

增加成员函数：

```cpp
// 处理客户端请求报文，在Connection类中回调此函数
void onmessage(Connection *conn,std::string message);

void TcpServer::onmessage(Connection *conn,std::string message)
{
        message = "repay:" + message;
        // 把message里内容发送给客户端
        int len = message.size();
        std::string tmpbuf((char*)&len,4); // 把报文头部填充到回应报文中
        tmpbuf.append(message);

        send(conn->fd(),tmpbuf.data(),tmpbuf.size(),0);
}
```



显然，该函数是给`Connection`调用的，因此，又要回调(下层`Connection`类调用上层`TcpServer`类)

步骤依然是(口述了)：

1. 在`TcpServer`里写好被回调的函数
2. 在`Connection`里写成员变量(回调函数)和设置该成员变量的成员函数
3. 在`TcpServer`的构造函数里，将被回调的函数与`Connection`类的成员变量`bind()`在一块
4. 在`Connection`的对应位置，调用回调函数即可

注意传参，注意设置占位符，(`bind*()`)绑定的位置不一定在构造函数，是根据在哪个函数里`new Connection`而决定

最后成功，不再展示 









cp -r 17 18

## 如何使用发送outputbuffer缓冲区？

send() 只是把数据写入**内核发送缓冲区**，如果发送缓冲区已满，send 可能只发送部分数据或返回 EAGAIN，因此需要**维护应用层发送缓冲区(outputbuffer)**并在 **EPOLLOUT** 事件中继续发送。

核心代码目前位于：

![](pc/75.png)

#### 1 为什么需要 EPOLLOUT

假设你要发送 **100KB 数据**：

```
send(fd, buf, 100*1024, 0);
```

但 **内核发送缓冲区只有 32KB 空间**。

于是 `send()` 可能返回：

```
32768
```

意思是：

```
只发送了 32KB ; 剩下 68KB 没发
```

这时候你有两个选择：

##### 错误方式

```
while(没发完)
    send()
```

问题：

```
发送缓冲区满 → send阻塞
整个服务器卡住
```

所以 **非阻塞服务器不能这样写**。

------

#### 2 正确方式（Reactor服务器）

正确做法是：

```
没发完的数据
↓
存到应用层发送缓冲区
↓
注册 EPOLLOUT
↓
等 socket 可写
↓
继续 send
```

------

#### 3 EPOLLOUT 是什么意思

`EPOLLOUT` 表示：

```
socket 发送缓冲区有空间了
可以继续 send
```

也就是：

```
内核通知你：
“可以继续发数据了”
```

------

#### 4 实际流程（服务器）

完整流程：

```
应用层数据
     ↓
send()
     ↓
发送缓冲区满
     ↓
剩余数据存入 outputBuffer
     ↓
epoll 监听 EPOLLOUT
     ↓
EPOLLOUT触发
     ↓
继续 send()
     ↓
发完后取消 EPOLLOUT
```

------





### 具体的代码修改部分

1. 在`Connection`类中，增加`send`函数，作用是把需要发送的数据保存在`Connection`类的`outputbuffer`中

```cpp
void send(const char* data,size_t size);

void Connection::send(const char* data,size_t size)
{
        outputbuffer_.append(data,size);
}
```

2. 注册写事件的代码如何写呢？

注册事件的代码在`Channel`类里，最初有一个：`void Channel::enablereading();`

现在添加：

![](pc/76.png)

分别是：

* 注册读事件
* 取消读时间
* 注册写事件
* 取消写事件

3. 补充`Channel`里的内容：
   ![](pc/77.png)

4. 开始写：当接收到`EPOLLOUT`事件时的代码，在`Channel::handleevent()`

![](pc/78.png)

在这里面修改

现在已经能把收到的信息放到`outputbuffer`并且能收到`EPOLLOUT`发送事件，那如何实现发出去呢？

由于`Connection`的发送缓冲区不属于`Channel`，因此，在这里无法访问，那就采用回调的方法：

具体步骤：

* 先在`Channel`里设置成员变量(回调函数)，然后写一个成员函数，用于设置该成员变量（给`Connection`调用，来`bind`它的函数）

* 在`Connection`类里写好`Channel`需要回调的函数
* 在`Connection`类的构造`Channel`对象的函数中，调用`Channel`成员函数，把需要回调的函数绑定上
* 在`Channel`类的对应位置调用成员变量（回调函数）

（代码在`18`中 ）



------

#### 5 为什么发完要取消 EPOLLOUT

因为：

```
socket几乎一直是可写的
```

如果不取消：

```
epoll会疯狂触发 EPOLLOUT
CPU 100%
```

所以：

```
只有数据没发完才监听 EPOLLOUT
```

这是 **高性能服务器经典设计**。

------

#### 7 最终结构（所有高性能服务器一样）

例如：

- Muduo
- Redis
- Nginx

架构都是：

```
应用缓冲区 (outputBuffer)
        ↓
send()
        ↓
内核发送缓冲区
```

如果发不完：

```
注册 EPOLLOUT
等待继续发送
```

------

#### 8 一句话总结

> **EPOLLOUT 的作用就是：当发送缓冲区有空间时通知程序继续发送未发送完的数据。**

