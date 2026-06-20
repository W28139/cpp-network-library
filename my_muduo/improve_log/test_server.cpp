#include <iostream>
#include <string>
#include <signal.h>
#include "my_muduo/TcpServer.h"
#include "my_muduo/EventLoop.h"
#include "my_muduo/Connection.h"

// 为了压测性能，预先定义好 HTTP 响应字符串，避免每次回调都进行字符串拼接
static std::string g_http_response = 
    "HTTP/1.1 200 OK\r\n"
    "Connection: keep-alive\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 12\r\n"
    "\r\n"
    "Hello World!";

// 处理新连接的回调
void OnNewConnection(spConnection conn) {
    // 生产环境下可以记录日志，压测时保持为空
}

// 处理消息的回调 (核心压测逻辑)
void OnMessage(spConnection conn, std::string &message) {
    // 1. 业务逻辑：收到 HTTP 请求后，直接发送预定义的响应
    // 此时 message 里存放的是 wrk 发来的 GET 请求内容
    
    // 2. 发送响应
    // 注意：这里的 conn->send 内部会调用我们优化过的 outputbuffer_.append()
    conn->send(g_http_response); 
}

// 处理连接关闭的回调
void OnClose(spConnection conn) {
    // 压测时不打印日志
}

// 错误处理回调
void OnError(spConnection conn) {
    // 压测时不打印日志
}

// 发送完成后的回调 (可选)
void OnSendComplete(spConnection conn) {
    // 可以在这里处理大文件分片发送逻辑
}

int main() {
    // 1. 忽略 SIGPIPE 信号
    // 在高并发网络编程中，如果客户端关闭了连接而服务器继续写，会触发 SIGPIPE 导致进程退出
    signal(SIGPIPE, SIG_IGN);

    // 2. 初始化 TcpServer
    // 参数：监听地址，端口，线程数
    // 建议线程数设为 CPU 核心数，压测效果最好
    TcpServer server("0.0.0.0", 8080, 4);

    // 3. 注册回调函数
    server.setnewconnectioncb(OnNewConnection);
    server.setonmessagecb(OnMessage);
    server.setcloseconnectioncb(OnClose);
    server.seterrorconnectioncb(OnError);
    server.setsendcompletecb(OnSendComplete);

    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "  Benchmark Server is running on port 8080      " << std::endl;
    std::cout << "  Thread num: 4                                 " << std::endl;
    std::cout << "  Press Ctrl+C to stop                          " << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    // 4. 启动服务器（内部会调用 loop.loop() 进入事件循环）
    server.start();

    return 0;
}