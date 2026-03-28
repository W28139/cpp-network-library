
#pragma once
#include<vector>
#include<queue>
#include<unistd.h>
#include<sys/syscall.h>
#include<mutex>
#include<unistd.h>
#include<thread>
#include<condition_variable>
#include<functional>
#include<future>
#include<atomic>

class ThreadPool
{
private:
	std::vector<std::thread> threads_;		// 线程池中的线程
	std::queue<std::function<void()>>taskqueue_;	// 任务队列
	std::mutex mutex_;				// 任务队列同步的互斥锁
	std::condition_variable condition_;		// 任务队列同步的条件变量
	std::atomic_bool stop_;				// 在析构函数中，把stop_变为true,全部线程退出
	std::string threadtype_;			// IO 或者 WORKS
public:
	
	// 在析构函数中，将启动threadnum线程
	ThreadPool(size_t threadnum,const std::string &threadtype);	
	
	// 把任务添加到队列中	
	void addtask(std::function<void()>task);

	// 在析构函数中停止线程
	~ThreadPool();
};
