#include "stdafx.h"

#include "Cng.h"

#pragma comment(lib, "bcrypt.lib")

namespace cng
{
	//
	// Smart handler of a BCRYPT_ALG_HANDLE
	//

	AlgHandle::AlgHandle()
	{
	}

	AlgHandle::AlgHandle(const std::wstring& alg, DWORD flags /*= 0*/)
	{
		status_ = ::BCryptOpenAlgorithmProvider(&handle_, alg.c_str(), nullptr, flags);
		if (FAILED(status_))
		{
			//LMLOG_DIAGNOSE(L"CNG Algorithm %s not valid, status: 0x%08X\n", alg.c_str(), status_);
		}
	}

	AlgHandle::~AlgHandle()
	{
		Release();
	}

	void AlgHandle::Release()
	{
		if (handle_ != NULL)
		{
			status_ = ::BCryptCloseAlgorithmProvider(handle_, 0);
			handle_ = NULL;
		}
	}

	bool AlgHandle::SetProperty(const std::wstring& prop, unsigned char* buf, DWORD len)
	{
		status_ = ::BCryptSetProperty(handle_, prop.c_str(), buf, len, 0);
		return SUCCEEDED(status_);
	}

	bool AlgHandle::SetProperty(const std::wstring& prop, DWORD val)
	{
		DWORD len = sizeof(DWORD);
		return SetProperty(prop, (unsigned char*)&val, len);
	}

	bool AlgHandle::SetProperty(const std::wstring& prop, const std::wstring& val)
	{
		DWORD len = (DWORD)(val.size() * sizeof(wchar_t));
		return SetProperty(prop, (unsigned char*)val.c_str(), len);
	}

	bool AlgHandle::GetPropertyLength(const std::wstring& prop, DWORD& len)
	{
		status_ = ::BCryptGetProperty(handle_, prop.c_str(), NULL, 0, &len, 0);
		return SUCCEEDED(status_);
	}

	bool AlgHandle::GetProperty(const std::wstring& prop, unsigned char* buf, DWORD& len)
	{
		status_ = ::BCryptGetProperty(handle_, prop.c_str(), buf, len, &len, 0);
		return SUCCEEDED(status_);
	}

	bool AlgHandle::GetProperty(const std::wstring& prop, DWORD& val)
	{
		DWORD len = sizeof(DWORD);
		return GetProperty(prop, (unsigned char*)&val, len);
	}

	bool AlgHandle::GetProperty(const std::wstring& prop, std::wstring& val)
	{
		DWORD len = 0;
		if (!GetPropertyLength(prop, len))
			return false;

		val.resize((len + 1) / sizeof(wchar_t), 0);
		return GetProperty(prop, (unsigned char*)val.c_str(), len);
	}

	//
	// Smart handler of a BCRYPT_HASH_HANDLE
	//

	HashHandle::HashHandle()
	{
	}

	HashHandle::~HashHandle()
	{
		Release();
	}

	void HashHandle::Release()
	{
		if (handle_ != NULL)
		{
			status_ = ::BCryptDestroyHash(handle_);
			handle_ = NULL;
		}
	}

}
