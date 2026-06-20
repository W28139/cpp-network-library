# Muduo 项目更新日志：高性能 ThreadPool 模块重构与 IO/Work 分离架构升级

**更新模块**：`mymuduo::ThreadPool` & `mymuduo::TcpServer`
**更新版本**：v2.0
**更新目标**：支持动态弹性伸缩，实现 IO 与业务逻辑彻底解耦，保障跨线程通信安全。

---

## 1. 核心功能升级

### 双模式运行支持 
引入 `PoolMode` 枚举，使线程池能够根据角色自适应：
*   **MODE_FIXED (固定模式)**：线程数恒定。专门用于 **IO 线程池**，每个线程绑定一个 `EventLoop`。由于 Reactor 模型的特性，IO 线程必须常驻，严禁动态销毁（防止连接丢失）。
*   **MODE_CACHED (弹性模式)**：根据任务压力动态增减。引入 `kMaxIdleTime(60s)` 回收机制。适用于 **Work 业务线程池**，在突发高并发计算时扩容，在空闲时释放 CPU 资源。

### 线程安全与生命周期管理
*   **从 Detach 到 Join 的转变**：废弃危险的 `detach`，改用 `unordered_map<int, thread>` 维护句柄。
*   **Safe-Shutdown 机制**：利用原子变量 `isPoolRunning_` 和条件变量 `notify_all()`，确保在程序退出时，所有 Work 线程能完成当前任务并安全阻塞回收，彻底杜绝进程退出时的野指针崩溃。
*   **跨线程对象生命周期保护**：在 Work 线程调用 `conn->send()` 时，强制通过 `shared_from_this()` 捕获当前连接对象并投递给 IO 线程，确保回调执行时对象未被析构。

---

## 2. 架构优化：IO 与 Work 彻底分离 

### 引入“线程归属(Thread Affinity)”设计
为了消除缓冲区竞态，本项目确立了以下核心准则：
1.  **IO 线程独占 IO 权**：所有 `send()`、`recv()`、`epoll_ctl()` 操作必须在所属 IO 线程的 `EventLoop` 内完成。
2.  **Work 线程纯计算化**：Work 线程仅负责业务逻辑/协议解析，禁止直接操作 Socket。
3.  **自动降级机制**：若用户未开启 Work 线程池，业务回调将自动退化到 IO 线程执行，保持框架的高灵活性。

### 跨线程任务投递流 (Task Dispatching)
通过 `runInLoop` 与 `eventfd` 唤醒机制，实现了无锁化的跨线程通信：
*   当 Work 线程完成计算调用 `conn->send(data)` 时，框架会自动判断当前线程环境。
*   若非 IO 线程，则将发送任务封装为 `Functor` 存入 `pendingTasks_` 队列，并通过 `eventfd` 瞬时唤醒 IO 线程完成最终非阻塞写入。

---

## 3. 线程角色与模式关系矩阵

| 角色类型 | 推荐模式 | 线程数量 | 核心职责 | 为什么不能混合？ |
| :--- | :--- | :--- | :--- | :--- |
| **IO 线程** | **FIXED** | 固定 (如 CPU 核心数) | 执行 `epoll_wait`，负责数据收发与定时器管理。 | 若 IO 线程被业务计算阻塞，将导致大量连接请求超时，系统吞吐量暴跌。 |
| **Work 线程** | **CACHED** | 动态 (根据业务负载) | 执行耗时算法、数据库查询、第三方 API 调用。 | 业务逻辑具有不确定性，采用 CACHED 模式可按需弹性伸缩资源。 |

---

## 4. 业务侧使用指南

用户现在可以通过极简的代码配置不同的并发模型：

### 场景 A：高性能小包转发（仅 IO 模式）
默认情况下，不开启 Work 线程池，所有操作在 IO 线程完成，延迟最低。
```cpp
mymuduo::TcpServer server("0.0.0.0", 8080, 8); // 8 个 IO 线程
server.start();
```

### 场景 B：高负载业务计算（IO + Work 分离模式）
一行代码开启业务线程池，支持动态扩缩容，自动实现计算与 IO 解耦。
```cpp
mymuduo::TcpServer server("0.0.0.0", 8080, 4); // 4 个 IO 线程

// 开启 16 个初始 Work 线程，采用 CACHED 模式，最大扩容到 100 线程
server.enableWorkPool(16, mymuduo::PoolMode::MODE_CACHED); 
server.setWorkThreadThreshold(100);

server.start();
```

### 业务回调内部编写
用户无需关注线程切换，直接调用 `send` 即可：
```cpp
void OnMessage(const ConnectionPtr& conn, std::string& msg) {
    // 此时代码可能运行在 WORKER-N 线程
    std::string result = DoHeavyCompute(msg); // 模拟耗时计算
    
    // 跨线程安全发送：底层自动将 data 投递回所属 IO 线程，无竞争修改 outputBuffer_
    conn->send(result); 
}
```

---

## 5. 调试与观测性 
*   **内核级命名**：通过 `prctl` 将线程重命名为 `IO_LOOP-N` 或 `WORKER-N`。
*   **监控利器**：在生产环境下，可通过 `top -H -p [pid]` 实时观察 IO 线程与 Work 线程的 CPU 占用差异，从而精准调整 `threadNum` 比例，实现真正的**性能调优**。

---

### 修改点补充说明：
1.  **修正**：在 `TcpServer` 中，`ioThreadPool_` 应该始终锁定为 `MODE_FIXED`。
2.  **修正**：在 `Connection::send()` 中，利用 `shared_from_this()` 解决 Work 线程向 IO 线程投递任务时的生命周期安全性。
3.  **理解点**：Work 线程**从不**竞争 `outputBuffer_`。因为它不直接修改它，它只是写一个“请帮我修改”的任务单（Task）塞进 IO 线程的队列里，由 IO 线程串行处理。

### 最后强调一个问题就是；
onmessage里有IO操作，并且会竞争outputBuffer_缓冲区？具体怎么避免的？
之前代码很清楚，现在做了一些调整，解释如下：
代码中，已设置outputBuffer_只出现在sendInLoop() 和 handleWrite() 里，当work进程里调用send()，即发生I/O操作时，会判断是否在work，如果在就把任务放进队列，通过events信号唤醒在I/O线程中执行
#### 具体的规避流程（以 Work 线程调用 send 为例）

当业务代码（Work 线程）调用 `conn->send(message)` 时，发生了以下精准的调度：

*   **第一步：判定身份**
    `send()` 函数内部第一行就是 `if (loop_->isInLoopThread())`。Work 线程在这里执行，判定结果一定是 `false`。
*   **第二步：打包任务**
    由于不是 IO 线程，Work 线程**绝对不会**去操作 `outputBuffer_`，也不会去调用 `::send` 系统调用。它只是把 `message` 和 `sendInLoop` 函数打包成一个回调（Functor）。
*   **第三步：跨线程投递**
    Work 线程调用 `loop_->runInLoop(...)`。这个函数会把打包好的任务塞进 `EventLoop` 的 `pendingTasks_` 队列。
    *   **注意：** 这里确实有竞争，但竞争的是 `pendingTasks_` 队列（通过 `mutex_` 保护），**而不是 `outputBuffer_`**。
*   **第四步：瞬时唤醒**
    Work 线程通过 `eventfd` 写入一个 8 字节的值。这会让阻塞在 `epoll_wait` 的 IO 线程立刻跳出。
*   **第五步：执行权回归**
    IO 线程被唤醒后，执行 `doPendingTasks()`，从中取出 Work 线程投递的任务并执行 `sendInLoop()`。
    *   此时，代码已经在 IO 线程里跑了。它去修改 `outputBuffer_` 或调用 `::send` 是绝对安全的，因为同一个 Loop 的其他任务（如 `handleWrite`）也一定是在这个线程里串行执行的。

#### 为什么“在 Work 线程里调用 send”不叫 I/O 操作？

从严谨的角度看，你在 Work 线程调用 `conn->send()` 时，**并没有发生真正的 I/O 操作**。

*   **Work 线程的行为：** 内存拷贝（把数据拷给闭包） + 任务入队 + 信号通知。
*   **IO 线程的行为：** 真正的 `::send` 系统调用 + `outputBuffer_` 写入。