# 用unique_ptr管理自己的资源

在上一节分析中，已知`Connection`对象必须用`shared_ptr`管理，其他对象可以用原始指针，也可以用智能指针`unique_ptr`去管理

因此，本节将原始指针全部改为智能指针，采用自底层向上层去修改

首先是InetAddress类与Socket类，这俩都没指针

下一个是Channel类，它的成员变量事件循环是指针，但该指针是在构造函数中，由外界传进来的，而非自己的，暂时不处理

下一个是Acceptor类，它有三个成员函数是原始指针，其中一个是外界传进来的事件循环，暂时不用修改，另外两个是在构造函数中创建的，因此可以修改，`Socket* serversock_和Channel* acceptchannel_`，由于这两个对象占用的空间很小，因此不用指针，放在栈内存里

![](pc/98.png)



下一个就到了`COnnection`类了

其中与`Acceptor`类相同，但不同的是，他的`Socket* clientsock_`是外界传进来的，不过生命周期是在`Connection`类内管理，它本身代码比较复杂，占用空间较大，因此用智能指针管理

![](pc/99.png)

虽然在这里销毁，但毕竟是传入进来的，因此在创建它的地方，也需要修改

![](pc/100.png)

由于给回调函数传参是智能指针，发生变化，因此也要去修改被回调的函数，即`Tcpserver`里的成员函数，如下：

![](pc/101.png)



接下来，继续修改`Connection`类里的普通指针，为`Channel* clientsock_`对象

但不清楚它的大小，那怎么处理？在main函数里，用sizeof()打印出看看，结果显示是160字节，一个`Connection`就有一个`Channel*`对象，如果有几百万连接，那内存就很大，因此还是用智能指针

![](pc/102.png)

当然，在`Accpetor`里也有`Channel`对象，但它放置的位置是栈内存，因为一个网络服务器就只有一个`Acceptor`,因此占用的内存会很小



接下来看Epoll类，发现没有指针，下一个是EventLoop类，存在Epoll指针，一个事件循环用一个`Epoll`，一个网络服务器，也就用几个事件循环（与CPU内核相关），因此用栈内存也没关系

接下来将`EventLoop`类的成员变量`Epoll* ep_`改为栈，即`Epoll ep_`

当发生报错：` error: field ‘ep_’ has incomplete type ‘Epoll’`

原因是头文件的互相包含与类的前置声明造成：

在`Epoll.h`中，包含了`Channel.h`，在`Channel.h`中，包含了`EventLoop.h`，在`EventLoop.h`中，又包含了`Epoll.h`，因此无法直接改用栈空间，此处改为智能指针即可



在网上层，就到了 TcpServer类

先把主事件循环改为`std::unique_ptr<EventLoop> mainloop_;`

![](pc/103.png)

接下来修改从事件循环，改为智能指针，如下：

![](pc/104.png)



现在主事件循环和从事件循环都改为了`unique_ptr`，而`Connection`和`Channel`中，都用到了事件循环，这几个类只是使用事件循环，对事件循环的资源没有所有权(不会delete资源)，所以在这几个类中，继续使用普通指针也可以（当然也可以改为智能指针），但在某些大佬看来，用普通指针就可以，可读性更高

接下来，我们将其改为智能指针

![](pc/105.png)

注意，`Acceptor`对其没有所有权，不能使用移动语义，因此加const 使用从引用

（此时我还没系统学智能指针，因此不太明白，后期补）

同样，`Channel`类也是，其构造函数里的成员函数也要同步改为智能指针，改完之后，之前`TcpServer`类成员函数里创建这俩类调用构造函数时，传入的是智能指针，但当时`Channel`和`Acceptor`的构造函数需要传参是普通指针，因此`.get()`此时可以删除这个`.get()`了



同样，我们把`Connection`里的事件循环，也改为智能指针



到此，与事件循环相关的已经全部改为智能指针，现在接着修改`TcpServer`

一个`TcpServer`只有一个`Acceptor`，因此把这个存入栈，不用指针(线程池也是，改为栈)



最后到`EchoServer`类，其中没有普通指针，不需要修改