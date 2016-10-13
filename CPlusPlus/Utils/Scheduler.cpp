#include "stdafx.h"
#include "Scheduler.h"
#include "Utils.h"

using namespace std;

namespace utils
{
	namespace thread_management
	{
		Scheduler::Scheduler(std::shared_ptr<ThreadPool> pThreadPool, const std::chrono::system_clock::duration& minRecurringInterval, const unsigned int& maxTaskSize, const std::chrono::system_clock::duration& maxDelayTolerance)
			:stop_(false),
			pThreadPool_(pThreadPool),
			maxTaskSize_(maxTaskSize),
			ready_(false),
			minRecurringInterval_(minRecurringInterval),
			maxDelayTolerance_(maxDelayTolerance)
		{
			Setup();
		}

		Scheduler::~Scheduler()
		{
			Stop();
		}

		void Scheduler::Setup()
		{
			if (pEnqueueThread_)
			{
				//Already created
				return;
			}

			if (!pThreadPool_)
			{
				//Log error, there is pThreadPool created
				//throw exception
				return;
			}

			pEnqueueThread_ = make_shared<thread>([this]()
			{
				while (!stop_.load())
				{
					auto now = std::chrono::system_clock::now();
					{
						unique_lock<mutex> lock(mtx_);
						while (!tasksQueue_.empty() && tasksQueue_.top().startTime_ <= now)
						{
							auto topTimeAndIntvervalAndTask = tasksQueue_.top();
							shared_ptr<Task> pNextTask;
							chrono::system_clock::time_point nextStartTime;
							auto skipCurrentOccurence = false;

							//Set up next occurrence
							if (topTimeAndIntvervalAndTask.recurringInterval_ != chrono::seconds(0))
							{		
								pNextTask = make_shared<Task>(*topTimeAndIntvervalAndTask.pTask_.get());
								auto passDuration = now - topTimeAndIntvervalAndTask.startTime_;
								auto remainder = chrono::duration_cast<chrono::milliseconds>(passDuration).count() % chrono::duration_cast<chrono::milliseconds>(topTimeAndIntvervalAndTask.recurringInterval_).count();
								nextStartTime = now + ((remainder < chrono::duration_cast<chrono::milliseconds>(minRecurringInterval_).count()) ? (topTimeAndIntvervalAndTask.recurringInterval_ + chrono::milliseconds(remainder)) : chrono::milliseconds(remainder));

								//For the recurring task, the default max delay tolerance is 1 second
								skipCurrentOccurence = passDuration > maxDelayTolerance_;							
							}

							if (!skipCurrentOccurence)
							{
								if (pThreadPool_->Enqueue(topTimeAndIntvervalAndTask.pTask_))
								{
									tasksQueue_.pop();
									if (pNextTask)
									{
										tasksQueue_.push(TimeAndIntervalAndTask(pNextTask, topTimeAndIntvervalAndTask.recurringInterval_, nextStartTime));
									}
								}
								else
								{
									//Log fail to enqueue
									this_thread::sleep_for(chrono::microseconds(200));
								}
							}
							else
							{
								tasksQueue_.pop();
								if (pNextTask)
								{
									tasksQueue_.push(TimeAndIntervalAndTask(pNextTask, topTimeAndIntvervalAndTask.recurringInterval_, nextStartTime));
								}
							}
						}

						if (tasksQueue_.empty())
						{
							if (!ready_.load())
							{
								ready_ = true;
							}

							cdv_.wait(lock);
						}
						else
						{
							cdv_.wait_until(lock, tasksQueue_.top().startTime_);
						}
					}
				}
			});

			while (!ready_.load())
			{
				this_thread::sleep_for(chrono::microseconds(200));
			}
		}

		void Scheduler::Stop()
		{
			if (stop_.load())
			{
				//return if already stop
				return;
			}

			//Stop Equeue Thread
			stop_ = true;
			cdv_.notify_all();
			if (pEnqueueThread_)
			{
				pEnqueueThread_->join();
			}

			//Stop ThreadPool
			if (pThreadPool_)
			{
				pThreadPool_->Stop();
			}
		}

		bool Scheduler::RunRecurringTaskAt(const std::chrono::system_clock::time_point& startTime, std::chrono::system_clock::duration recurringInterval, std::shared_ptr<Task> pTask)
		{
			if (!pTask)
			{
				//Log
				return false;
			}

			if (recurringInterval != chrono::seconds(0) && recurringInterval < minRecurringInterval_)
			{
				//Log
				return false;
			}

			// 100 years ago schedule task is not valid
			if ((chrono::system_clock::now() - startTime) > chrono::hours(100 * 365 * 24))
			{
				//Log
				return false;
			}

			{
				unique_lock<mutex> lock(mtx_);
				if (tasksQueue_.size() > maxTaskSize_)
				{
					//Log reach the max task size
					return false;
				}

				TimeAndIntervalAndTask timeAndRepeatIntervalAndTask(pTask, recurringInterval, startTime);

				tasksQueue_.push(timeAndRepeatIntervalAndTask);
			}

			cdv_.notify_one();

			return true;
		}	

		bool Scheduler::RunTaskAt(std::chrono::system_clock::time_point startTime, std::shared_ptr<Task> pTask)
		{
			if (startTime < (chrono::system_clock::now() - maxDelayTolerance_))
			{
				//Log the none-recurring task start time is in the pass, the task should be schduled from now on
				return false;
			}
			return RunRecurringTaskAt(startTime, chrono::seconds(0), pTask);
		}
	}
}