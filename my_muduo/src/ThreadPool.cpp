#include"my_muduo/ThreadPool.h"

/*
	std::vector<std::thread> threads_;		// 线程池中的线程
	std::queue<std::function<void()>>taskqueue_;	// 任务队列
	std::mutex mutex_;				// 任务队列同步的互斥锁
	std::condition_variable condition_;		// 任务队列同步的条件变量
	std::atomic_bool stop_;				// 在析构函数中，把stop_变为true,全部线程退出
	std::string threadtype_;			// IO 或者 WORKS
*/

ThreadPool::ThreadPool(size_t threadnum,const std::string& threadtype):stop_(false),threadtype_(threadtype)
{
	for(size_t ii=0;ii<threadnum;ii++)
	{
		threads_.emplace_back([this]()
		{

		// 显示线程ID
			printf("create %s thread(%ld).\n",threadtype_.c_str(),syscall(SYS_gettid));
			while(stop_==false)
			{
				std::function<void()>task;
			
				{
					std::unique_lock<std::mutex> lock(this->mutex_);
					// 如果lambda返回true,那就往下执行，否则就阻塞wait
					this->condition_.wait(lock,[this]()
					{
						return (this->stop_==true) || (this->taskqueue_.empty()==false);
					});
					// 判断是否打烊了
					if((this->stop_==true) && (this->taskqueue_.empty()==true))
					{
						return;
					}
					// 有新任务
					task = std::move(this->taskqueue_.front());
					this->taskqueue_.pop();
				}
				// printf("%s(%ld)execute task.\n",threadtype_.c_str(),syscall(SYS_gettid));
				task();
			}
		});	
	}
}

void ThreadPool::addtask(std::function<void()>task)
{
	{
		std::lock_guard<std::mutex>lock(mutex_);
		taskqueue_.push(task);
	}
	condition_.notify_one();
}

ThreadPool::~ThreadPool()
{
	stop();
}

size_t ThreadPool::size()
{
	return threads_.size();
}

void ThreadPool::stop()
{
	if(stop_) return;
	stop_ = true;
	condition_.notify_all();

	for(std::thread &th:threads_)
	{
		th.join();
	}
}