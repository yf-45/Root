#pragma once

#include <vector>

#include <bcrypt.h>

// from DDK
#define NT_SUCCESS(Status)          (((NTSTATUS)(Status)) >= 0)
#define STATUS_UNSUCCESSFUL         ((NTSTATUS)0xC0000001L)

namespace cng
{
	//
	// Base smart handler class
	//
	template<typename _T> class Base
	{
		Base(const Base&);

	protected:
		_T		handle_;
		DWORD	status_;

		Base() :
			handle_(NULL),
			status_((DWORD)STATUS_UNSUCCESSFUL)
		{
		}

		virtual void Release() = 0;

	public:
		virtual ~Base() {};

		operator _T()
		{
			return handle_;
		}

		_T* operator&()
		{
			return &handle_;
		}

		DWORD Status() const
		{
			return status_;
		}

		virtual _T Swap(_T newHandle)
		{
			_T oldHandle = handle_;
			handle_ = newHandle;
			status_ = 0;
			return oldHandle;
		}

		virtual void Attach(_T newHandle)
		{
			Release();
			Swap(newHandle);
		}

		virtual _T Detach()
		{
			return Swap(NULL);
		}
	};

	//
	// Smart handler of a BCRYPT_ALG_HANDLE
	//
	class AlgHandle : public Base<BCRYPT_ALG_HANDLE>
	{
	public:
		AlgHandle();
		AlgHandle(const std::wstring& alg, DWORD flags = 0);
		virtual ~AlgHandle();
		virtual void Release();

		bool SetProperty(const std::wstring& prop, unsigned char* buf, DWORD len);
		bool SetProperty(const std::wstring& prop, DWORD val);
		bool SetProperty(const std::wstring& prop, const std::wstring& val);

		bool GetPropertyLength(const std::wstring& prop, DWORD& len);
		bool GetProperty(const std::wstring& prop, unsigned char* buf, DWORD& len);
		bool GetProperty(const std::wstring& prop, DWORD& val);
		bool GetProperty(const std::wstring& prop, std::wstring& val);
	};

	//
	// Smart handler of a BCRYPT_HASH_HANDLE
	//
	class HashHandle : public Base<BCRYPT_HASH_HANDLE>
	{
	public:
		HashHandle();
		virtual ~HashHandle();
		virtual void Release();
	};
}
