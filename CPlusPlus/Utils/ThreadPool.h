#pragma once

namespace utils
{
	namespace thread_management
	{
		class ThreadPool
		{
		public:
			//by default, keep last 1,000 executed tasks to be queryable.
			//thread::hardware_concurrency();
			ThreadPool(unsigned int poolSize = std::thread::hardware_concurrency(), const std::wstring& name = L"defaultThreadPool", const unsigned int& maxQueryableExecutedTaskSize = 10000, const unsigned int& maxPendingTaskSize = 1000000, const int& priority = THREAD_PRIORITY_NORMAL);
			~ThreadPool();

			void Setup();
			bool Enqueue(std::shared_ptr<Task> task);
			std::wstring GetName() const;
			unsigned int GetPoolSize() const;
			unsigned long long GetExecutedTaskCount() const;
			unsigned long long GetEnqueuedTaskCount() const;
			unsigned long long GetPendingTaskCount() const;
			//std::vector<std::shared_ptr<Task>> GetPendingTasks() const;
			std::vector<std::shared_ptr<Task>> GetExceptionTasks() const;
			std::vector<std::shared_ptr<Task>> GetExecutedTasks() const;

			//Wait for all running tasks to complete and then quit the worker threads.
			void Stop();

		protected:
			std::mutex queueMutex_;
			mutable SRWLOCK srwLock_;
			int priority_;

			std::condition_variable cdv_;
			std::vector<std::shared_ptr<std::thread>> threads_;

			std::deque<std::shared_ptr<Task>> taskQueue_;
			std::deque<std::shared_ptr<Task>> executedTasks_;
			std::deque<std::shared_ptr<Task>> exceptionTasks_;
			std::wstring name_;
			unsigned int maxQueryableExecutedTaskSize_;
			//unsigned int maxEnqueableTaskSize_;
			unsigned int maxPendingTaskSize_;
			std::atomic<unsigned int> poolSize_;			
			std::atomic<bool> stop_;
			std::atomic<unsigned long long> enqueuedTaskCount_;
			std::atomic<unsigned long long> executedTaskCount_;
			std::atomic<unsigned int> readyThreadCount_;
		};
	}
}