#pragma once
#include "Task.h"
#include "ThreadPool.h"
namespace utils
{
	namespace thread_management
	{
		struct TimeAndIntervalAndTask
		{
			TimeAndIntervalAndTask(std::shared_ptr<Task> pTask, const std::chrono::system_clock::duration& recurringInterval, const std::chrono::system_clock::time_point& startTime)
				: pTask_(pTask),
				recurringInterval_(recurringInterval),
				startTime_(startTime)
			{}		

			std::shared_ptr<Task> pTask_;
			std::chrono::system_clock::duration recurringInterval_;
			std::chrono::system_clock::time_point startTime_;			
		};

		struct TimeAndIntervalAndTaskSorter
		{
			bool operator()(const TimeAndIntervalAndTask& left, const TimeAndIntervalAndTask& right)
			{
				return left.startTime_ > right.startTime_;
			}
		};		

		class Scheduler
		{
		public:
			Scheduler(std::shared_ptr<ThreadPool> pThreadPool, const std::chrono::system_clock::duration& minRecurringInterval = std::chrono::milliseconds(100), const unsigned int& maxTaskSize = 1000000, const std::chrono::system_clock::duration& maxDelayTolerance_ = std::chrono::milliseconds(1000));
			~Scheduler();

			bool RunTaskAt(std::chrono::system_clock::time_point startTime, std::shared_ptr<Task> pTask);			
			bool RunRecurringTaskAt(const std::chrono::system_clock::time_point& startTime, std::chrono::system_clock::duration recurringInterval, std::shared_ptr<Task> pTask);

		private:
			void Setup();
			void Stop();

			std::priority_queue<TimeAndIntervalAndTask, std::vector<TimeAndIntervalAndTask>, TimeAndIntervalAndTaskSorter> tasksQueue_;
			std::shared_ptr<ThreadPool> pThreadPool_;
			std::shared_ptr<std::thread> pEnqueueThread_;
			std::atomic<bool> stop_;
			std::atomic<bool> ready_;

			std::mutex mtx_;
			std::condition_variable cdv_;
			unsigned int maxTaskSize_;
			std::chrono::system_clock::duration minRecurringInterval_;
			std::chrono::system_clock::duration maxDelayTolerance_;
		};
	}
}