#pragma once

#include <memory>
#include <list>

#ifndef	lenof
#define	lenof(x)	( sizeof(x) / sizeof((x)[0]) )
#endif

#include "Helper.h"

namespace utils
{

#pragma region NoneTemplate

	std::wstring ToWstring(const std::string& str, const std::locale& loc = std::locale());

	std::string ToString(const std::wstring& str, const std::locale& loc = std::locale());

	std::string Utf16ToUtf8(const std::u16string& utf16);

	std::u16string Utf8ToUtf16(const std::string& utf8);

	std::u16string WstringToU16str(const std::wstring& wstr);

	std::wstring U16strToWstring(const std::u16string& u16str);

	std::string WstringToUtf8(const std::wstring& wstr);

	std::wstring Utf8ToWstring(const std::string& utf8);

	//format string are in https://msdn.microsoft.com/en-us/library/fe06s4ak.aspx
	std::string FormatTm(const std::string& format, const tm & time);

	std::string GetNow();

	std::string GetISO8601(const FILETIME& t, bool utc = false);
	
	std::string GetNowISO8601(bool utc = false);

	bool ExtractISO8601(const std::string& value, FILETIME* pFileTime, bool* pIsUTC);

	std::string GetTid();

	bool FileExists(const std::wstring& path, long long* pSize = nullptr);

	bool DirectoryExists(const std::wstring& path);

	std::vector<std::wstring> ToSmallerWstrings(std::wstring wstr, const unsigned int& length);

	bool GetRegStringValue(std::wstring& value, const std::wstring& subKey, const std::wstring& valueName, HKEY root);

	bool IsServiceRunning(const std::wstring& serviceName);

	bool ToTimePoint(std::chrono::system_clock::time_point& tp, std::tm tm, bool utc = false);

	bool ToTm(std::tm& tm, const std::chrono::system_clock::time_point& tp, bool utc = false);	
	
	bool ToTimePoint(std::chrono::system_clock::time_point& tp, const std::string& iso8601Str);

	//bool ToIso8601Str(const std::chrono::system_clock::time_point& tp, string& iso8601Str);

	std::list<std::wstring> FindDirectories(const std::wstring& folder, const std::wstring& pattern = L"*");

	std::list<std::wstring> FindFiles(const std::wstring& folder, const std::wstring& pattern = L"*");

	std::wstring GetGuid();

	std::wstring GetTemp();

	int DeleteDirectory(const std::wstring &rootDirectory, bool bDeleteSubdirectories = true);

	bool IoThrottleSupported();

	//DWORD GetEnvironmentVariableW(const std::wstring& name, std::wstring& out);

	std::string ToBase64(const void* buf, size_t len);

	std::vector<unsigned char> FromBase64(const std::string& base64);

#pragma endregion

#pragma region Templates
	template<typename TContent = std::wstring, typename TPath = std::wstring>
	bool ReadFile(const TPath& path, TContent& str)
	{
		str.clear();

		typedef std::remove_const<std::remove_pointer<decltype(str.data())>::type>::type ValueType;

		std::basic_ifstream<ValueType, std::char_traits<ValueType>> ifs;
		ifs.open(path.c_str(), std::ios_base::in | std::ios_base::binary);

		if (!ifs.is_open())
		{
			return false;
		}

		try
		{
			ValueType c;
			while (ifs.get(c))
			{
				//TODO: Max memory check, default 32bit user mode is 2GB
				//auto MaxSize = 2 * 1024 * 1024 * 1024;
				str.push_back(c);
			}

			if (!ifs.eof())
			{
				ifs.close();
				return false;
			}
		}
		catch (...)
		{
			ifs.close();
			return false;
		}

		ifs.close();
		return true;
	}

	template<typename TContent = std::wstring, typename TPath = std::wstring>
	bool AppendToFile(const TPath& path, const TContent& str)
	{
		typedef std::remove_const<std::remove_pointer<decltype(str.data())>::type>::type ValueType;

		if (str.size() == 0)
		{
			return false;
		}

		std::basic_ofstream<ValueType, std::char_traits<ValueType>> ofs(path.c_str(), std::ios_base::out | std::ios_base::app | std::ios_base::binary);

		if (!ofs.is_open())
		{
			return false;
		}

		ofs.write(str.c_str(), str.size());
		ofs.close();

		return true;
	}
	// convert buffer to its hexadecimal shape
	template <typename T> T ToHex(const void* buf, size_t len)
	{
		T hexstr = T(2 * len, '\0');
		for (size_t i = 0, j = 0; i < len; ++i)
		{
			unsigned char b = *((unsigned char*)(buf)+i);
			T::value_type c = (b & 0xF0) >> 4;
			hexstr[j++] = c > 9 ? (c - 9) | 0x40 : c | 0x30;
			c = b & 0xF;
			hexstr[j++] = c > 9 ? (c - 9) | 0x40 : c | 0x30;
		}
		return std::move(hexstr);
	}

	// convert string to its hexadecimal shape
	template <typename T> T ToHex(const T& str)
	{
		return std::move(ToHex<T>(str.c_str(), str.size() * sizeof(T::value_type)));
	}

	// fill buffer from hexadecimal shape
	template <typename T> size_t FromHex(const T& hexstr, void* buf, size_t len)
	{
		size_t fixedLen = __min((hexstr.size() + 1) / 2, len);
		for (size_t i = 0; i < fixedLen; ++i)
		{
			unsigned char c = (unsigned char)hexstr[i << 1];
			unsigned char v = (c > '9' ? c + 9 : c) << 4;
			c = (unsigned char)hexstr[(i << 1) | 1];
			v |= (c > '9' ? (c + 9) & 0xF : c & 0xF);
			((unsigned char*)buf)[i] = v;
		}
		return fixedLen;
	}

	// convert hexadecimal shape back to string
	template <typename T> T FromHex(const T& hexstr)
	{
		T str = T((hexstr.size() + 1) / 2 / sizeof(T::value_type), '\0');
		size_t len = FromHex(hexstr, &str[0], str.size() * sizeof(T::value_type));
		return std::move(str);
	}

	template <typename T> bool IsValidHexa(const T& hexstr)
	{
		char validChars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'A', 'b', 'B', 'c', 'C', 'd', 'D', 'e', 'E', 'f', 'F', 0 };
		T valid(validChars, validChars + strlen(validChars));
		return hexstr.find_first_not_of(valid) == T::npos;
	}

	template <typename T> T Trim(const T& s)
	{
		return TrimRight(TrimLeft(s));
	}

	template <typename T> T TrimLeft(const T& s)
	{
		T::value_type unwanted[] = { ' ', '\n', '\r', '\t', 0 };
		T::size_type startpos = s.find_first_not_of(StringUtils::WHITESPACE);
		return (startpos == T::npos) ? T() : s.substr(startpos);
	}

	template <typename T> T TrimRight(const T& s)
	{
		T::value_type unwanted[] = { ' ', '\n', '\r', '\t', 0 };
		T::size_type endpos = s.find_last_not_of(StringUtils::WHITESPACE);
		return (endpos == T::npos) ? T() : s.substr(0, endpos + 1);
	}
#pragma endregion
}