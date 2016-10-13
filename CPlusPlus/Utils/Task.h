#pragma once

namespace utils
{
	namespace thread_management
	{
		enum TaskState
		{
			NotStarted,
			Complete,
			Running,
			Terminated
		};

		const std::string TaskStateStr[] =
		{
			"NotStarted",
			"Complete",
			"Running",
			"Terminated"
		};

		class Task
		{
		public:			
			Task(std::function<void()> action, const std::string& name = "", const std::string& guid = "", std::function<void()> pCallback = std::function<void()>());
			~Task();

			void SetState(const TaskState& state);
			void SetErrorMessage(const std::string& message);

			TaskState GetState() const;
			std::string GetName() const;
			//std::string GetGuid() const;
			//void SetErrorMessage(const std::string& message) const;
			std::string GetErrorMessage() const;
			std::chrono::system_clock::time_point GetStartTime() const;
			std::chrono::system_clock::time_point GetCompleteTime() const;
			std::function<void()> GetAction() const;
			std::function<void()> GetCallback() const;
			void Run();

			Task& operator=(const Task& rhs);
			Task(const Task& rhs);

		private:
			std::string name_;
			std::string guid_;
			std::chrono::system_clock::time_point startTime_;
			std::chrono::system_clock::time_point completeTime_;
			std::string errorMessage_;
			TaskState state_;
			std::function<void()> action_;
			std::function<void()> callback_;
			mutable SRWLOCK srwLock_;
			mutable std::mutex mutex_;
		};
	}
}