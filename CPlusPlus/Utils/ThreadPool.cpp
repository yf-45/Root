#include "stdafx.h"
#include "Task.h"
#include "Helper.h"
#include "ThreadPool.h"

using namespace std;

namespace utils
{
	namespace thread_management
	{
		ThreadPool::ThreadPool(unsigned int poolSize, const wstring& name, const unsigned int& maxQueryableExecutedTaskSize, const unsigned int& maxPendingTaskSize, const int& priority)
			:
			poolSize_(poolSize),
			name_(name),
			priority_(priority),
			maxQueryableExecutedTaskSize_(maxQueryableExecutedTaskSize),			
			maxPendingTaskSize_(maxPendingTaskSize),
			stop_(false),
			enqueuedTaskCount_(0),
			executedTaskCount_(0),
			readyThreadCount_(0)
		{
			InitializeSRWLock(&srwLock_);
			Setup();
		}

		ThreadPool::~ThreadPool()
		{
			Stop();
		}

		void ThreadPool::Stop()
		{

			if (stop_.load())
			{
				//return if already stopped
				return;
			}

			stop_ = true;

			//Signal Stop to all running threads
			cdv_.notify_all();

			//Wait for the running task to be finished
			for (auto thread : threads_)
			{
				thread->join();
			}
		}

		void ThreadPool::Setup()
		{
			for (unsigned int i = 0; i < poolSize_.load(); i++)
			{
				auto th = make_shared<thread>([this]()
				{
					auto hThread = ::GetCurrentThread();
					auto currentPriority = ::GetThreadPriority(hThread);				

					if (priority_ != currentPriority)
					{
						auto result = ::SetThreadPriority(hThread, priority_);
						if (!result)
						{
							throw runtime_error("Fail to set up thread priority");						
						}
					}

					try
					{
						shared_ptr<Task> pTask;
						while (!stop_.load())
						{
							{
								// acquire lock
								unique_lock<mutex> lock(queueMutex_);

								// look for a work item
								while (!stop_.load() && taskQueue_.empty())
								{
									if (readyThreadCount_.load() < poolSize_.load())
									{
										readyThreadCount_++;
									}
									cdv_.wait(lock);
								}

								// exit if the pool is stopped
								if (stop_.load())
								{
									return;
								}

								// get the task from the queue
								pTask = taskQueue_.front();

								taskQueue_.pop_front();

							}// release lock							

							try
							{
								pTask->Run(); // execute the task								
							}
							catch (std::exception& ex)
							{
								//log ex
								UNREFERENCED_PARAMETER(ex);
								utils::WriteLock lock(srwLock_);
								//Only keep 1000 exception tasks
								if (exceptionTasks_.size() > 1000)
								{
									exceptionTasks_.pop_front();
								}

								exceptionTasks_.push_back(pTask);
							}
							catch (...)
							{
								//log exception
								utils::WriteLock lock(srwLock_);
								if (exceptionTasks_.size() > 1000)
								{
									exceptionTasks_.pop_front();
								}
								exceptionTasks_.push_back(pTask);
							}

							executedTaskCount_++;

							{
								utils::WriteLock lock(srwLock_);
								if (executedTasks_.size() > maxQueryableExecutedTaskSize_)
								{
									executedTasks_.pop_front();
								}

								executedTasks_.push_back(pTask);
							}
						}
					}
					catch (std::exception& ex)
					{
						//log fatal error
						UNREFERENCED_PARAMETER(ex);
					}
					catch (...)
					{
						//Log fatal error
					}
				});

				threads_.push_back(th);			
			}

			while (readyThreadCount_.load() != poolSize_.load())
			{
				Sleep(200);
			}
		}

		bool ThreadPool::Enqueue(shared_ptr<Task> task)
		{
			if (GetPendingTaskCount() > maxPendingTaskSize_)
			{
				//log wait for a monment to allow the thread pool execute existing pending tasks				
				return false;
			}

			{
				unique_lock<mutex> lock(queueMutex_);
				taskQueue_.push_back(task);
			}

			enqueuedTaskCount_++;
			cdv_.notify_one();
			return true;
		}

		std::vector<std::shared_ptr<Task>> ThreadPool::GetExceptionTasks() const
		{
			utils::ReadLock lock(srwLock_);
			return std::move(std::vector<std::shared_ptr<Task>>(exceptionTasks_.begin(), exceptionTasks_.end()));
		}

		std::vector<std::shared_ptr<Task>> ThreadPool::GetExecutedTasks() const
		{
			utils::ReadLock lock(srwLock_);
			return std::move(std::vector<std::shared_ptr<Task>>(executedTasks_.begin(), executedTasks_.end()));			
		}

		unsigned long long ThreadPool::GetEnqueuedTaskCount() const
		{
			return enqueuedTaskCount_.load();
		}

		unsigned long long ThreadPool::GetExecutedTaskCount() const
		{
			return executedTaskCount_.load();
		}

		unsigned long long ThreadPool::GetPendingTaskCount() const
		{
			return enqueuedTaskCount_.load() - executedTaskCount_.load();
		}

		wstring ThreadPool::GetName() const
		{
			return name_;
		}

		unsigned int ThreadPool::GetPoolSize() const
		{
			return poolSize_;
		}
	}
}