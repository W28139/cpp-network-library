# 基于 Reactor 模式的 Muduo 库规范化重构与性能演进报告

## 1. 项目综述
本阶段任务是对自研的 C++ 网络库进行深度重构。重构目标涵盖**代码工程规范化**、**现代 C++ 特性适配**、**线程安全增强**以及**高并发性能优化**。通过这一系列修改，系统在 `wrk` 压力测试中表现卓越，单机吞吐量稳定在 **57 万 QPS** 以上。

## 2. 规范化与工程化改进

### 2.1 命名与视觉规范
*   **统一命名法**：全面推行 Google C++ 风格。类名采用 `PascalCase`（如 `TcpServer`），函数名采用 `camelCase`（如 `onMessage`），成员变量统一增加下划线后缀 `variableName_`。
*   **命名空间封装**：将所有核心组件封装在 `mymuduo` 命名空间内，有效防止全局符号污染。

### 2.2 现代 C++ 特性引入
*   **智能指针管理**：弃用裸指针，全面改用 `std::unique_ptr` 管理具有独占权的资源（如 `Socket`, `Channel`, `EventLoop`），利用 `std::shared_ptr` 管理连接对象生命周期。
*   **RAII 机制强化**：通过 `std::enable_shared_from_this` 确保 `Connection` 在异步回调期间的内存安全。
*   **禁止拷贝逻辑**：对核心类显式禁用拷贝构造和赋值操作符，防止资源重复释放。

---

## 3. 核心组件深度重构（Component Refactoring）

### 3.1 Buffer 模块

### 3.2 EventLoop 模块：任务队列并发优化
*   **临界区优化**：重构 `doPendingTasks` 逻辑。采用 `std::queue::swap` 技术，将任务队列快速交换到局部变量执行，使锁的持有时间仅为指针交换的时间，避免了在执行回调时阻塞其他线程投递任务。
*   **唤醒机制**：使用 `eventfd` 替代传统的管道，实现了更轻量级的跨线程唤醒。

### 3.3 Epoll & Channel 模块：职责解耦
*   **接口标准化**：将 `Epoll::loop` 重命名为 `Epoll::poll`，逻辑上更贴近系统原义。
*   **状态透明化**：为 `Channel` 增加 `isWriting`, `isReading` 等查询接口，使 `Connection` 能够根据事件状态执行“直接发送”优化。
*   **CLOEXEC 适配**：所有系统调用（`epoll_create1`, `accept4`）均显式开启 `SOCK_CLOEXEC`，防止多进程环境下的文件描述符泄露。

### 3.4 Connection & TcpServer：多 Reactor 调度
*   **连接生命周期管理**：通过 `handleClose` 链路闭环，确保 `TcpServer` 容器、`EventLoop` 监视器以及业务回调之间的状态同步。
*   **多线程分发策略**：完善 `TcpServer` 的轮询（Round-Robin）算法，将 `Acceptor` 接收的连接均匀分布到 `subLoops` 中，实现真正的 Multi-Reactor 负载均衡。

---

## 4. 性能压测验证（Benchmark）

重构后，通过 `wrk` 工具进行 HTTP Echo 压力测试：

*   **测试环境**：12 线程并发 / 400 保持连接 / 30 秒持续运行。
*   **压测数据记录**：
    ```text
    17295020 requests in 30.10s, 1.63GB read
    Requests/sec: 574585.48
    Transfer/sec: 55.34MB
    Latency (Avg): 708.25us
    ```
*   **结果分析**：
    *   **吞吐量**：达到了 **57.4 万 QPS**，证明重构后的非阻塞 I/O 和 Buffer 机制能极高效率地处理短连接请求。
    *   **稳定性**：Latency 的标准差较小，表明 `EventLoop` 的任务调度和 `Epoll` 的事件分发在高负荷下依然稳定。

---

## 5. 修改日志摘要 (ChangeLog Summary)
1.  **[ADD]** 引入 `mymuduo` 命名空间。
2.  **[REF]** Buffer 
3.  **[FIX]** 修复 `localtime`、`inet_ntoa` 等非线程安全函数的使用，改为 `_r` 和 `ntop` 版本。
4.  **[OPT]** `Connection::send` 性能优化：当输出缓冲区为空时尝试直接 `send` 绕过 `EPOLLOUT`。
5.  **[OPT]** `EventLoop` 任务执行优化：锁内 `swap` 任务队列，缩小临界区。
6.  **[ENH]** 全局忽略 `SIGPIPE` 信号，提升高并发写操作的健壮性。