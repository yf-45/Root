#pragma once

namespace utils
{
	class smart_handle : public std::unique_ptr < std::remove_pointer<HANDLE>::type, decltype(&::CloseHandle) >
	{
	public:
		smart_handle(HANDLE h) : std::unique_ptr<std::remove_pointer<HANDLE>::type, decltype(&::CloseHandle)>(h, &::CloseHandle) {}
	};

	class smart_module : public std::unique_ptr < std::remove_pointer<HMODULE>::type, decltype(&::FreeLibrary) >
	{
	public:
		smart_module(HMODULE h) : std::unique_ptr<std::remove_pointer<HMODULE>::type, decltype(&::FreeLibrary)>(h, &::FreeLibrary) {}
	};

	class SlimReadWriteLock
	{
		SRWLOCK lock_;
	public:
		SlimReadWriteLock();
		~SlimReadWriteLock();

		operator SRWLOCK&();
	};

	class ReadLock
	{
		SRWLOCK& lock_;
	public:
		ReadLock(SRWLOCK& lock);
		~ReadLock();

		ReadLock& operator=(const ReadLock& rhs) = delete;
		ReadLock(const ReadLock& rhs) = delete;
	};

	class WriteLock
	{
		SRWLOCK& lock_;
	public:
		WriteLock(SRWLOCK& lock);
		~WriteLock();

		WriteLock& operator=(const WriteLock& rhs) = delete;
		WriteLock(const WriteLock& rhs) = delete;
	};
}