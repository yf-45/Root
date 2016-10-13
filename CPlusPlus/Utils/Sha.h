#pragma once

#include <vector>

namespace sha
{
	enum algid_t
	{
		sha1_160,
		sha2_224,
		sha2_256,
		sha2_384,
		sha2_512,
		sha3_224,
		sha3_256,
		sha3_384,
		sha3_512,
	};

	bool GetAlgorithmId(const std::wstring& name, algid_t& algid, bool& implemented, size_t& sizeInBytes);

	bool GetAlgorithmName(algid_t algid, std::wstring& name, bool& implemented, size_t& sizeInBytes);

	std::vector<unsigned char> HashBuffer(algid_t algid, const void* buffer, size_t len);

	std::vector<unsigned char> HashFile(algid_t algid, const std::wstring& filename);
}
