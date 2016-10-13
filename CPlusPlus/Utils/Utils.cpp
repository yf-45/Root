#include "stdafx.h"
#include "Utils.h"
#include <malloc.h>
#include <stdio.h>
#include <objbase.h>
#include <atlenc.h>

namespace utils
{
	using namespace std;

	namespace
	{
		const char timestamp_format[] = "%4hd-%02hd-%02hdT%02hd:%02hd:%02hd.%03hd";
		const char utc_postfix[] = "Z";
	}

	wstring ToWstring(const string& str, const locale& loc)
	{
		if (str.empty())
		{
			return wstring();
		}

		vector<wchar_t> buf(str.size());
		use_facet<ctype<wchar_t>>(loc).widen(str.data(), str.data() + str.size(), buf.data());
		return wstring(buf.data(), buf.size());
	}

	string ToString(const wstring& str, const locale& loc)
	{
		if (str.empty())
		{
			return string();
		}

		vector<char> buf(str.size());
		use_facet<ctype<wchar_t>>(loc).narrow(str.data(), str.data() + str.size(), '?', buf.data());
		return string(buf.data(), buf.size());
	}

	string Utf16ToUtf8(const u16string& utf16)
	{
		if (utf16.empty())
		{
			return string();
		}

		// The UTF-8 / UTF-16 standard conversion facet
		wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> conversion;
		return conversion.to_bytes(utf16);
	}

	u16string Utf8ToUtf16(const string& utf8)
	{
		if (utf8.empty())
		{
			return u16string();
		}

		// The UTF-8 / UTF-16 standard conversion facet
		wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> conversion;
		return conversion.from_bytes(utf8);
	}

	wstring U16strToWstring(const u16string& u16str)
	{
		wstring wstr(u16str.begin(), u16str.end());
		return wstr;
	}

	u16string WstringToU16str(const wstring& wstr)
	{
		u16string u16str(wstr.begin(), wstr.end());
		return u16str;
	}

	std::string WstringToUtf8(const std::wstring& wstr)
	{
		return Utf16ToUtf8(WstringToU16str(wstr));
	}

	std::wstring Utf8ToWstring(const std::string& utf8)
	{
		return U16strToWstring(Utf8ToUtf16(utf8));
	}

	std::string FormatTm(const std::string& format, const tm & time)
	{
		const int max = 100;

		char str[max] = { 0 };

		auto size = strftime(str, max, format.c_str(), &time);

		if (size > 0)
		{
			return string(str);
		}

		return string();
	}

	string GetNow()
	{
		time_t t = time(0);   // get time now
		tm now;
		localtime_s(&now, &t);		
		return FormatTm("%Y-%m-%dT%H:%M:%S", now);
	}

	string GetISO8601(const FILETIME& t, bool utc /*= false*/)
	{
		SYSTEMTIME st = { 0 };
		char buffer[64] = { 0 };
		if (::FileTimeToSystemTime(&t, &st))
		{
			sprintf_s(buffer, timestamp_format,
				st.wYear, st.wMonth, st.wDay,
				st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

		if (utc)
		{
				strcat_s(buffer, utc_postfix);
		}
		}

		return std::move(std::string(buffer));
	}

	string GetNowISO8601(bool utc /*= false*/)
	{
		FILETIME ft = { 0 }, lft = { 0 };
		if (FAILED(::CoFileTimeNow(&ft)) || !utc && !::FileTimeToLocalFileTime(&ft, &lft))
		{
			return "";
		}
		return std::move(GetISO8601(utc ? ft : lft, utc));
		}

	bool ExtractISO8601(const std::string& value, FILETIME* pFileTime, bool* pIsUTC)
	{
		SYSTEMTIME st = { 0 };
		int res = sscanf_s(value.c_str(), timestamp_format, &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond, &st.wMilliseconds);
		if (res < 6) // ignore missing millisec then 6 or 7 are fine
		{
			return false;
		}
		if (pFileTime != nullptr)
		{
			::SystemTimeToFileTime(&st, pFileTime);
		}
		if (pIsUTC != nullptr)
		{
			*pIsUTC = _stricmp(&value[value.size() - strlen(utc_postfix)], utc_postfix) == 0;
		}
		return true;
	}

	string GetTid()
	{
		stringstream ss;
		ss << ::GetCurrentThreadId();
		return ss.str();
	}

	bool FileExists(const std::wstring& path, long long* pSize /*= nullptr*/)
	{
		_stat64 buffer = { 0 };
		int res = _wstat64(path.c_str(), &buffer);
		bool isFile = res >= 0 && ((buffer.st_mode & 0x4000 /*_S_IFDIR*/) == 0);
		if (pSize != nullptr && isFile)
		{
			*pSize = buffer.st_size;
		}
		return isFile;
	}

	bool DirectoryExists(const std::wstring& path)
	{
		_stat64 buffer = { 0 };
		int res = _wstat64(path.c_str(), &buffer);
		return res >= 0 && ((buffer.st_mode & 0x4000 /*_S_IFDIR*/) != 0);
	}

	vector<wstring> ToSmallerWstrings(wstring wstr, const unsigned int& length)
	{
		vector<wstring> elems;
		//size_t pos = 0;
		wstring token;
		while (length < wstr.size())
		{
			token = wstr.substr(0, length);
			elems.push_back(token);
			wstr.erase(0, length);
		}

		if (wstr.size() > 0)
		{
			elems.push_back(wstr);
		}

		return elems;
	}

	bool GetRegStringValue(wstring& value, const wstring& subKey, const std::wstring& valueName, HKEY root)
	{
		HKEY hKey;
		auto result1 = RegOpenKeyEx(root, subKey.c_str(), 0, KEY_READ, &hKey);
		if (result1 != ERROR_SUCCESS)
		{
			//fail to RegOpenKeyEx
			return false;
		}

		DWORD cbData = 512;
		DWORD dwRet;
		std::unique_ptr<wchar_t> szBuffer;

		dwRet = RegQueryValueExW(hKey,
			valueName.c_str(),
			NULL,
			NULL,
			NULL,
			&cbData);

		if (dwRet == ERROR_SUCCESS)
		{
			szBuffer = std::unique_ptr<wchar_t>(new wchar_t[cbData + 1]{0});
			dwRet = RegQueryValueExW(hKey,
				valueName.c_str(),
				NULL,
				NULL,
				(LPBYTE)szBuffer.get(),
				&cbData);

			if (dwRet != ERROR_SUCCESS)
			{
				RegCloseKey(hKey);
				return false;
			}

			value = szBuffer.get();
			RegCloseKey(hKey);
			return true;
		}

		RegCloseKey(hKey);
		return false;
	}

	bool IsServiceRunning(const std::wstring& serviceName)
	{
		auto seviceRunning = false;

		SC_HANDLE scmHandle = OpenSCManager(0, 0, GENERIC_READ);

		if (scmHandle)
		{
			SC_HANDLE scHandle = OpenService(scmHandle, serviceName.c_str(), GENERIC_READ);
			if (!scHandle)
			{
				if (ERROR_SERVICE_DOES_NOT_EXIST != GetLastError())
				{
					//fprintf(stderr, "Failed to OpenService(): %d\n", GetLastError());				
				}
				else
				{
					/* Service does not exist. */
					//fprintf(stderr, "Service does not exist.\n");					
				}
			}
			else
			{
				//fprintf(stderr, "Opened service.\n");

				SERVICE_STATUS serviceStatus;

				if (ControlService(scHandle, SERVICE_CONTROL_INTERROGATE, &serviceStatus))
				{
					if (serviceStatus.dwCurrentState == SERVICE_RUNNING)
					{
						seviceRunning = true;
					}
				}

				CloseServiceHandle(scHandle);
			}

			CloseServiceHandle(scmHandle);
		}
		else
		{
			//fprintf(stderr, "Failed to OpenSCManager(): %d\n", GetLastError());
		}

		return seviceRunning;
	}




	////
	bool ToTimePoint(std::chrono::system_clock::time_point& tp, std::tm tm, bool utc)
	{
		//tm.tm_isdst = -1;
		std::time_t tt = utc ? _mkgmtime(&tm) : std::mktime(&tm);
		if (tt == -1)
		{
			//Not valid system time
			return false;
		}
		tp = chrono::system_clock::from_time_t(tt);
		return true;
	}

	bool ToTm(std::tm& tm, const std::chrono::system_clock::time_point& tp, bool utc)
	{
		auto timeT = std::chrono::system_clock::to_time_t(tp);		
		if (!utc)
		{
			if (localtime_s(&tm, &timeT) != 0)
			{
				return false;
			}
		}
		else
		{
			if (gmtime_s(&tm, &timeT) != 0)
			{
				return false;
			}
		}

		return true;	
	}

	bool ToTimePoint(chrono::system_clock::time_point& tp, const string& iso8601Str)
	{
		SYSTEMTIME st = { 0 };
		int res = sscanf_s(iso8601Str.c_str(), timestamp_format, &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond, &st.wMilliseconds);
		if (res < 6) // ignore missing millisec then 6 or 7 are fine
		{
			return false;
		}

		tm tm;
		tm.tm_year = st.wYear - 1900;
		tm.tm_mon = st.wMonth - 1;
		tm.tm_mday = st.wDay;
		tm.tm_hour = st.wHour;
		tm.tm_min = st.wMinute;
		tm.tm_sec = st.wSecond;
		tm.tm_isdst = -1;

		chrono::system_clock::time_point tempTp;
		if (!ToTimePoint(tempTp, tm, iso8601Str.back() == 'Z'))
		{
			return false;
		}

		tp = tempTp + chrono::milliseconds(st.wMilliseconds);

		return true;
	}

	static std::list<std::wstring> Find(const std::wstring& folder, const std::wstring& pattern, bool directories)
	{
		std::list<std::wstring> res;

		HANDLE hFind = INVALID_HANDLE_VALUE;
		BOOL   fMore = TRUE;
		WIN32_FIND_DATA fndData = { 0 };

		std::wstring searchPattern = folder + L"\\" + pattern;
		for (hFind = ::FindFirstFile(searchPattern.c_str(), &fndData);
			hFind != INVALID_HANDLE_VALUE && fMore == TRUE;
			fMore = FindNextFile(hFind, &fndData))
		{
			if ((fndData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
				(wcscmp(fndData.cFileName, L".") == 0 || wcscmp(fndData.cFileName, L"..") == 0))
				continue;	// ignore . and ..

			if (directories == ((fndData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0))
			{
				res.push_back(fndData.cFileName);
			}
		}

		if (hFind != INVALID_HANDLE_VALUE)
			::FindClose(hFind);

		return std::move(res);
	}

	std::string ToBase64(const void* buf, size_t len)
	{
		std::string encoded(2 * len + 4, 0);
		int destLen = encoded.size();
		if (FALSE == Base64Encode((const unsigned char*)buf, len, &encoded[0], &destLen))
		{
			destLen = 0;
		}
		encoded.resize(destLen);
		return std::move(encoded);
	}

	std::vector<unsigned char> FromBase64(const std::string& base64)
	{
		std::vector<unsigned char> decoded(base64.size(), 0);
		int destLen = decoded.size();
		if (FALSE == Base64Decode(base64.c_str(), base64.size(), &decoded[0], &destLen))
		{
			destLen = 0;
		}
		decoded.resize(destLen);
		return std::move(decoded);
	}

	std::list<std::wstring> FindDirectories(const std::wstring& folder, const std::wstring& pattern /*= L"*"*/)
	{
		return std::move(Find(folder, pattern, true));
	}

	std::list<std::wstring> FindFiles(const std::wstring& folder, const std::wstring& pattern /*= L"*"*/)
	{
		return std::move(Find(folder, pattern, false));
	}

	std::wstring GetGuid()
	{
		GUID guid;
		if (SUCCEEDED(CoCreateGuid(&guid)))
		{
			OLECHAR* bstrGuid;
			if (SUCCEEDED(StringFromCLSID(guid, &bstrGuid)))
			{
				wstring guidWstr = bstrGuid;
				::CoTaskMemFree(bstrGuid);
				return guidWstr;
			}
		}

		return L"";
	}

	wstring GetTemp()
	{
		std::wstring strTempPath(L"C:\\Windows\\Temp");
		wchar_t wchPath[MAX_PATH];
		if (::GetTempPath(MAX_PATH, wchPath))
		{
			strTempPath = wchPath;
		}

		return strTempPath;
	}

	int DeleteDirectory(const std::wstring &rootDirectory, bool bDeleteSubdirectories)
	{
		bool            bSubdirectory = false;       // Flag, indicating whether
													 // subdirectories have been found
		HANDLE          hFile;                       // Handle to directory
		std::wstring     strFilePath;                 // Filepath
		std::wstring     strPattern;                  // Pattern
		WIN32_FIND_DATA FileInformation;             // File information


		strPattern = rootDirectory + L"\\*.*";
		hFile = ::FindFirstFileW(strPattern.c_str(), &FileInformation);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (FileInformation.cFileName[0] != '.')
				{
					strFilePath.erase();
					strFilePath = rootDirectory + L"\\" + FileInformation.cFileName;

					if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						if (bDeleteSubdirectories)
						{
							// Delete subdirectory
							int iRC = DeleteDirectory(strFilePath, bDeleteSubdirectories);
							if (iRC)
							{
								return iRC;
							}
						}
						else
						{
							bSubdirectory = true;
						}
					}
					else
					{
						// Set file attributes
						if (::SetFileAttributes(strFilePath.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
						{
							return ::GetLastError();
						}
						// Delete file
						if (::DeleteFile(strFilePath.c_str()) == FALSE)
						{
							return ::GetLastError();
						}
					}
				}
			} while (::FindNextFile(hFile, &FileInformation) == TRUE);

			// Close handle
			::FindClose(hFile);

			DWORD dwError = ::GetLastError();
			if (dwError != ERROR_NO_MORE_FILES)
			{
				return dwError;
			}
			else
			{
				if (!bSubdirectory)
				{
					// Set directory attributes
					if (::SetFileAttributes(rootDirectory.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
					{
						return ::GetLastError();
					}

					// Delete directory
					if (::RemoveDirectory(rootDirectory.c_str()) == FALSE)
					{
						return ::GetLastError();
					}
				}
			}
		}

		return 0;
	}

	bool IoThrottleSupported()
	{
		auto hThread = ::GetCurrentThread();
		auto result = ::SetThreadPriority(hThread, THREAD_MODE_BACKGROUND_BEGIN);
		if (!result)
		{
			return false;
		}
		else
		{
			result = ::SetThreadPriority(hThread, THREAD_MODE_BACKGROUND_END);
			return result != 0;
		}
	}
}