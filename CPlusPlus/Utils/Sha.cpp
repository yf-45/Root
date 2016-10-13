#include "stdafx.h"

#include "Sha.h"
#include "Cng.h"
#include "Utils.h"
#include <fstream>
#include <iostream>

namespace sha
{
	namespace details
	{
		static struct
		{
			algid_t algid_;
			const wchar_t* name_;
			bool implemented_;
			size_t sizeInBytes_;
		}
		mappings[] =
		{
			{ sha1_160, BCRYPT_SHA1_ALGORITHM,		true,	160 / 8 },
			{ sha2_224, L"SHA224",					false,	224 / 8 },
			{ sha2_256, BCRYPT_SHA256_ALGORITHM,	true,	256 / 8 },
			{ sha2_384, BCRYPT_SHA384_ALGORITHM,	true,	384 / 8 },
			{ sha2_512, BCRYPT_SHA512_ALGORITHM,	true,	512 / 8 },
			{ sha3_224, L"SHA3-224",				false,	224 / 8 },
			{ sha3_256, L"SHA3-256",				false,	256 / 8 },
			{ sha3_384, L"SHA3-384",				false,	384 / 8 },
			{ sha3_512, L"SHA3-512",				false,	512 / 8 },
		};
	}

	bool GetAlgorithmId(const std::wstring& name, algid_t& algid, bool& implemented, size_t& sizeInBytes)
	{
		for (size_t i = 0; i < lenof(details::mappings); ++i)
		{
			if (_wcsicmp(name.c_str(), details::mappings[i].name_) == 0)
			{
				algid = details::mappings[i].algid_;
				implemented = details::mappings[i].implemented_;
				sizeInBytes = details::mappings[i].sizeInBytes_;
				return true;
			}
		}
		return false;
	}

	bool GetAlgorithmName(algid_t algid, std::wstring& name, bool& implemented, size_t& sizeInBytes)
	{
		for (size_t i = 0; i < lenof(details::mappings); ++i)
		{
			if (algid == details::mappings[i].algid_)
			{
				name = details::mappings[i].name_;
				implemented = details::mappings[i].implemented_;
				sizeInBytes = details::mappings[i].sizeInBytes_;
				return true;
			}
		}
		return false;
	}

	std::vector<unsigned char> HashBuffer(algid_t algid, std::function<bool(const void*& buf, size_t& len)> feed)
	{
		std::wstring name;
		bool implemented;
		size_t sizeInBytes;
		if (GetAlgorithmName(algid, name, implemented, sizeInBytes))
		{
			cng::AlgHandle hHashAlg(name.c_str(), 0);
			if ((BCRYPT_ALG_HANDLE)hHashAlg != nullptr)
			{
				DWORD cbData = 0, cbHashObject = 0;
				NTSTATUS status = ::BCryptGetProperty(hHashAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0);
				if (NT_SUCCESS(status))
				{
					cng::HashHandle hHash;
					std::vector<unsigned char> temp(cbHashObject, 0);
					status = ::BCryptCreateHash(hHashAlg, &hHash, &temp[0], cbHashObject, nullptr, 0, 0);

					const void* buf = nullptr;
					size_t len = 0;
					while (NT_SUCCESS(status) && feed(buf, len))
					{
						status = ::BCryptHashData(hHash, (BYTE*)buf, (ULONG)len, 0);
						buf = nullptr;
						len = 0;
					}

					if (NT_SUCCESS(status))
					{
						std::vector<unsigned char> res(sizeInBytes, 0);
						status = ::BCryptFinishHash(hHash, &res[0], (ULONG)res.size(), 0);
						if (NT_SUCCESS(status))
						{
							return std::move(res);
						}
					}
				}
			}
		}
		return std::move(std::vector<unsigned char>());
	}

	std::vector<unsigned char> HashBuffer(algid_t algid, const void* buf, size_t len)
	{
		bool first = true;
		auto lambda = [&](const void*& buf_, size_t& len_) 
		{
			if (first)
			{
				buf_ = buf;
				len_ = len;
				first = false;
				return true;
			}
			return false;
		};
		return std::move(HashBuffer(algid, lambda));
	}

	std::vector<unsigned char> HashFile(algid_t algid, const std::wstring& filename)
	{
		// by chunks of 64KB
		const size_t capacity = 64 * 1024;

		std::vector<unsigned char> res;
		std::vector<char> buffer(capacity, 0);

		std::ifstream input;
		input.open(filename, std::ifstream::binary | std::ifstream::in);
		if (input.is_open())
		{
			auto lambda = [&](const void*& buf_, size_t& len_)
			{
				auto i = std::istreambuf_iterator<char>(input);
				auto j = input.readsome(&buffer[0], capacity);
				buf_ = &buffer[0];
				len_ = (size_t)j;
				return j > 0;
			};
			res = std::move(HashBuffer(algid, lambda));

			input.close();
		}

		return std::move(res);
	}
}