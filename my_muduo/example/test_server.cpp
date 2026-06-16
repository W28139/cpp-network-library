#include "my_muduo/TcpServer.h"
#include <iostream>
#include <string>
#include <functional>

// 1. 处理新连接的回调
void OnNewConnection(spConnection conn) {
    std::cout << "New Connection: " << conn->ip() << ":" << conn->port() << std::endl;
}

// 2. 处理消息的回调 (Echo 逻辑)
void OnMessage(spConnection conn, std::string &message) {
    std::cout << "Received: " << message; 
    
    // 把收到的消息原样发回
    conn->send(message); 
}

// 3. 处理连接关闭的回调
void OnClose(spConnection conn) {
    std::cout << "Connection Closed: " << conn->ip() << ":" << conn->port() << std::endl;
}

int main() {
    // 根据你的 TcpServer.h: TcpServer(const std::string &ip, const uint16_t port, int threadnum=3);
    // 我们监听 0.0.0.0:8080，使用 4 个线程
    TcpServer server("0.0.0.0", 8080, 4);

    // 使用你定义的 setter 函数名
    server.setnewconnectioncb(OnNewConnection);
    server.setonmessagecb(OnMessage);
    server.setcloseconnectioncb(OnClose);

    std::cout << "EchoServer is starting on 0.0.0.0:8080..." << std::endl;

    // 启动服务器
    // 注意：根据你的实现，如果 start() 内部调用了 mainloop_->loop()，它会在这里阻塞运行
    server.start();

    return 0;
}