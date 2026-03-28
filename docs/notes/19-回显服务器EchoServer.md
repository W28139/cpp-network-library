IO/20

## 回显服务器EchoServer

到目前为止，一个9个类，全部都是底层类，不会涉及业务，与业务没有关系

其中`TcpServer`是最上层的类，但依然属于底层类，属于网络库的一部分

`TcpServer`类里有很多通用代码，与业务无关，在哪个网络服务器都是一样的，为了支持不同业务，我们在`TcpServer`之上，创建业务类，实现业务需求



我们把`TcpServer`业务方面的内容提取到`EchoServer`类中

其中`EchoServer.h`为：

```cpp
#pragma once
#include"TcpServer.h"
#include"EventLoop.h"
#include"Connection.h"

class EchoServer
{
private:
        TcpServer tcpserver_;

public:
        EchoServer(const std::string &ip,const uint16_t port);
        ~EchoServer();

        void Start();

        void HandleNewConnection(Socket* clientsock);
        void HandleClose(Connection *conn);
        void HandleError(Connection *conn);
        void HandleSendComplete(Connection *conn);
        void HandleTimeOut(EventLoop *loop);
        void HandleMessage(Connection *conn,std::string message);

};
```

易发现需要很多回调函数，交给`TcpServer`调用

处理完那几个回调函数后，就没什么其他问题







