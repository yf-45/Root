#include "stdafx.h"
#include "Task.h"
#include "Utils.h"
#include "Helper.h"

using namespace std;

namespace utils
{
	namespace thread_management
	{
		Task::Task
			(
			std::function<void()> action,		
			const string& name,
			const string& guid,
			std::function<void()> callback
			)
			:
			action_(action),			
			name_(name),
			guid_(guid),
			state_(NotStarted),
			callback_(callback)
		{
			InitializeSRWLock(&srwLock_);
		}

		Task::Task(const Task& rhs)
			:
			name_(rhs.name_),
			guid_(rhs.guid_),
			startTime_(rhs.startTime_),
			completeTime_(rhs.completeTime_),
			errorMessage_(rhs.errorMessage_),
			state_(rhs.state_),
			action_(rhs.action_),
			callback_(rhs.callback_)
		{
			InitializeSRWLock(&srwLock_);
		}

		Task::~Task()
		{
		}

		Task& Task::operator = (const Task& rhs)
		{
			this->name_ = rhs.name_;
			this->guid_ = rhs.guid_;
			this->startTime_ = rhs.startTime_;
			this->completeTime_ = rhs.completeTime_;
			this->errorMessage_ = rhs.errorMessage_;
			this->state_ = rhs.state_;
			this->action_ = rhs.action_;
			this->callback_ = rhs.callback_;
			return *this;
		}

		void Task::Run()
		{
			unique_lock<mutex> lock(mutex_);
			try
			{
				startTime_ = chrono::system_clock::now();
				SetState(Running);
				if (action_)
				{
					action_();
				}

				SetState(Complete);
				completeTime_ = chrono::system_clock::now();
				//log task excueted
				if (callback_)
				{
					callback_();
				}
			}
			catch (exception& ex)
			{
				string errorMessage;

				if (GetState() != Complete)
				{
					SetState(Terminated);
				}
				else
				{
					errorMessage.append("Exception occurred when callback is called,");
				}

				errorMessage.append(ex.what());

				SetErrorMessage(errorMessage);

				throw;
			}
			catch (...)
			{
				string errorMessage;

				if (GetState() != Complete)
				{
					SetState(Terminated);
				}
				else
				{
					errorMessage.append("Exception occurred when callback is called,");
				}

				errorMessage.append("Unknown Error");

				SetErrorMessage(errorMessage);

				throw;
			}
		}

		void Task::SetErrorMessage(const std::string& message)
		{
			utils::WriteLock lock(srwLock_);
			errorMessage_ = message;
		}

		std::string Task::GetErrorMessage() const
		{
			utils::ReadLock lock(srwLock_);
			return errorMessage_;
		}

		TaskState Task::GetState() const
		{
			utils::ReadLock lock(srwLock_);
			return state_;
		}

		void Task::SetState(const TaskState& state)
		{
			utils::WriteLock lock(srwLock_);
			state_ = state;
		}

		string Task::GetName() const
		{
			utils::ReadLock lock(srwLock_);
			return name_;
		}

		chrono::system_clock::time_point Task::GetStartTime() const
		{
			utils::ReadLock lock(srwLock_);
			return startTime_;
		}

		chrono::system_clock::time_point Task::GetCompleteTime() const
		{
			utils::ReadLock lock(srwLock_);
			return completeTime_;
		}

		std::function<void()> Task::GetAction() const
		{
			return action_;
		}

		std::function<void()> Task::GetCallback() const
		{
			return callback_;
		}
	}
}