# High-Concurrency C++ Network Library

![C++](https://img.shields.io/badge/Language-C++11/14/17-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Linux-orange.svg)
![Model](https://img.shields.io/badge/Model-Reactor-green.svg)
![License](https://img.shields.io/badge/License-MIT-lightgrey.svg)

基于 C++ 实现的高并发网络库，采用 **Reactor 模型（epoll + 线程池）**，支持多线程并发处理。本项目旨在参考 **Muduo** 设计思想，构建一个轻量级、高性能、易于扩展的 TCP 网络框架。

---

## 📌 项目介绍

本项目是一个参考 Muduo 思想实现的轻量级网络库，底层基于 Linux `epoll`，结合 **IO线程 + 工作线程模型**，实现高并发 TCP 服务。

**适用于：**
- 🚀 深入理解 Linux 系统编程与网络协议栈。
- 🧠 学习 Reactor 事件驱动模型。
- 🏗️ 搭建高性能服务器（如 HTTP Server, RPC 框架）的基础底层框架。

---

## 🧠 核心特性

- **高效 IO 复用**：基于 Linux `epoll`（边缘触发 ET 模式可选）的多路复用。
- **经典 Reactor 模型**：采用多线程 Reactor 设计方案。
- **解耦设计**：IO 线程负责事件分发，工作线程池（ThreadPool）负责业务计算，避免阻塞 IO。
- **健壮性**：支持非阻塞 Socket、线程安全的任务调度及内存管理。
- **自动扩容**：封装 `Buffer` 类，支持动态扩容的读写缓冲区。

---

## 🏗️ 整体架构

### 模块关系图
```text
                +------------------+
                |   TcpServer      | (应用层入口)
                +--------+---------+
                         |
               +---------v---------+
               |    EventLoop      | (Reactor 核心)
               +---------+---------+
                         |
        +----------------+----------------+
        |                                 |
+-------v-------+                 +-------v-------+
|   Channel     |                 |   Connection  |
| (事件分发器)  |                 | (TCP 连接封装) |
+---------------+                 +---------------+
        |                                 |
+-------v-------+                 +-------v-------+
|    Epoll      |                 |    Buffer     |
| (IO 多路复用) |                 | (读写缓冲区)  |
+---------------+                 +---------------+
```

### ⚙️ 线程模型
1. **主线程 (Main Thread)**：负责 `Acceptor` 接收新连接。
2. **IO 线程 (EventLoop)**：
   - 监听文件描述符上的读写事件。
   - 负责数据的读取与发送。
3. **工作线程 (ThreadPool)**：
   - 执行计算密集型的业务逻辑。
   - 处理完成后，将结果写回 IO 线程发送。

---

## 📂 项目结构

```bash
.
├── Acceptor.{h,cpp}    # 封装监听套接字，负责 accept 新连接
├── TcpServer.{h,cpp}   # 对外接口，管理所有连接与线程池
├── EventLoop.{h,cpp}   # Reactor 核心，事件循环控制
├── Epoll.{h,cpp}       # 封装 epoll_wait/ctl，IO 复用层
├── Channel.{h,cpp}     # 负责文件描述符 fd 的事件分发
├── Connection.{h,cpp}  # 单个 TCP 连接的抽象（状态控制、收发数据）
├── Buffer.{h,cpp}      # 应用层读写缓冲区，解决粘包问题
├── ThreadPool.{h,cpp}  # 任务队列与工作线程池
├── Socket.{h,cpp}      # 套接字操作封装（RAII）
├── InetAddress.{h,cpp} # IP 和 Port 地址类
├── Timestamp.{h,cpp}   # 高精度时间戳工具
├── EchoServer.{h,cpp}  # 示例：回显服务器实现
├── echoserver.cpp      # 服务端启动程序
├── client.cpp          # 测试客户端
└── Makefile            # 构建脚本
```

---

## 🚀 快速上手

### 编译
要求：Linux 环境，`g++` 支持 C++11 或更高版本。
```bash
make
```

### 运行服务器
```bash
# 在 6789 端口启动 Echo 服务
./echoserver 127.0.0.1 6789
```

### 运行客户端
```bash
./client 127.0.0.1 6789
```

---

## 🧪 测试说明
- **Echo 测试**：发送任意字符串，服务器将实时返回相同数据。
- **并发连接测试**：支持多客户端同时在线，通过线程池均衡业务负载。
- **压力测试**：可使用 `wrk` 或自定义脚本进行高频并发请求测试。

---

## 🔥 项目说明

1. **从 0 到 1 实现 Reactor 模型**：深度理解事件循环、非阻塞 IO 与回调机制。
2. **手写 epoll 封装**：掌握 ET/LT 模式的区别及如何正确处理 `EAGAIN`。
3. **多线程解耦设计**：通过 `ThreadPool` 实现业务逻辑与 IO 读写的完全解耦。
4. **完善的内存管理**：使用 `std::shared_ptr` 和 `std::enable_shared_from_this` 管理 Connection 生命周期，防止悬空指针。
5. **缓冲区设计**：手写具备自动扩容功能的 `Buffer` 类，处理 TCP 粘包与半包问题。

---

## 📈 后续优化方向
- [ ] **HTTP 协议解析**：引入状态机解析 HTTP 请求。
- [ ] **定时器管理**：支持踢掉长时间不活跃的死连接。
- [ ] **零拷贝优化**：探索 `sendfile` 或更高效的 Buffer 管理。
- [ ] **CMake 集成**：使用 CMake 替代 Makefile 进行跨平台构建管理。
- [ ] **日志系统**：引入异步日志模块，记录服务器运行状态。

---

## 📚 参考资料
- 《Linux 多线程服务端编程》—— 陈硕 (Muduo 作者)
- 《UNIX 网络编程》卷1
- Reactor 模式详解

---

## 👤 作者
**[19hz]**
- GitHub: [@https://github.com/W28139]
- 个人主页: [https://github.com/W28139/cpp-network-library]

---

