### 用map容器管理Connection对象

1. **调整代码结构**

除了 `TcpServer`类，其他都属于底层架构， 而最接近业务的是 `TcpServer`，因此，需要调整一个日志，它应该属于业务，而非底层

![](pc/52.png)

因此将这一句话放置到`TcpServer`类内，在接收新连接，创建完成`Connection`对象后打印

但是有一个问题，就是`TcpServer::newconnection`里，取不到`clientaddr`的`ip`和`port`信息，因此我调整类`Socket`，增加成员变量`ip_、port_`和获取他们的成员函数，在具体实现的代码里，在`Socket::bind`和`Socket::accept`里，设置好成员变量的值；

同时，在`Connection`类里增加成员函数，用于获取`fd、ip、port` （从`Socket`类里调用返回，因为`Socket`类是`Connection`类的成员变量）

最终实现效果：

![](pc/53.png)



2. **优化**

在`TcpServer`类里，一个`TcpServer`类对应多个`Connection`类，因此，为了方便管理，以及解决前面遗留问题--新建立连接的`connection`没有销毁，我们在`TcpServer`类里添加成员变量`std::map<int,Connection*>conns_;`，第一个int代表fd，第二个代表连，此时就可以在析构函数里遍历，然后delete即可

添加和delete代码如下：
![](pc/54.png)

但此时依旧非最优`delete`，因为这是在程序关闭后统一delete,而不是在断开连接后即使单个关闭

此问题遗留（关于Connection类的生命周期问题）

