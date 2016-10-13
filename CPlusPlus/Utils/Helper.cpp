#include "stdafx.h"
#include "Helper.h"

namespace utils
{
	SlimReadWriteLock::SlimReadWriteLock()
	{
		InitializeSRWLock(&lock_);
	}

	SlimReadWriteLock::~SlimReadWriteLock()
	{
	}

	SlimReadWriteLock::operator SRWLOCK&()
	{
		return lock_;
	}

	ReadLock::ReadLock(SRWLOCK& lock)
		:lock_(lock)
	{
		AcquireSRWLockShared(&lock_);
	}

	ReadLock::~ReadLock()
	{
		ReleaseSRWLockShared(&lock_);
	}

	WriteLock::WriteLock(SRWLOCK& lock)
		:lock_(lock)
	{
		AcquireSRWLockExclusive(&lock_);
	}

	WriteLock::~WriteLock()
	{
		ReleaseSRWLockExclusive(&lock_);
	}
}