#include <sstream>
#include <filesystem>
#include "utils.h"
#include <windows.h>
#include <TlHelp32.h> 



namespace  utils
{

	uint32_t hex2dec(std::string hex)
	{
		std::stringstream ss2;
		uint32_t d2;
		ss2 << std::hex << hex; //选用十六进制输出
		ss2 >> d2;
		return d2;
	}

	// 12 -> C
	std::string dec2hex(uint64_t dec)
	{
		std::stringstream ss2;
		ss2 << std::hex << dec;
		return ss2.str();
	}

	// 12 -> 0000000C
	std::string dex2hex2(uint64_t dec)
	{
		char buffer[100]{};
		sprintf_s(buffer, "%p", dec);
		return buffer;
	}

	
	std::string utf2gbk(const char* utf8)
	{
		int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
		wchar_t* wstr = new wchar_t[len + 1];
		memset(wstr, 0, len + 1);
		MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
		len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
		char* str = new char[len + 1];
		memset(str, 0, len + 1);
		WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
		if (wstr) delete[] wstr;
		return str;
	}


	std::string gbk2utf8(std::string gbkStr)
	{
		std::string outUtf8 = "";
		int n = MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, NULL, 0);
		WCHAR* str1 = new WCHAR[n];
		MultiByteToWideChar(CP_ACP, 0, gbkStr.c_str(), -1, str1, n);
		n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);
		char* str2 = new char[n];
		WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, n, NULL, NULL);
		outUtf8 = str2;
		delete[]str1;
		str1 = NULL;
		delete[]str2;
		str2 = NULL;
		return outUtf8;
	}

	std::string wstring2stirng(std::wstring wstr)
	{
		std::string result;
		//获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的  
		int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
		char* buffer = new char[len + 1];
		//宽字节编码转换成多字节编码  
		WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
		buffer[len] = '\0';
		//删除缓冲区并返回值  
		result.append(buffer);
		delete[] buffer;
		return result;
	}

	std::wstring string2wstirng(std::string str)
	{
		std::wstring result;
		//获取缓冲区大小，并申请空间，缓冲区大小按字符计算  
		int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
		TCHAR* buffer = new TCHAR[len + 1];
		//多字节编码转换成宽字节编码  
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);
		buffer[len] = '\0';             //添加字符串结尾  
		result.append(buffer);
		delete[] buffer;
		return result;
	}

	std::string bytesToHexString(const BYTE* bytes, const int length)
	{
		if (bytes == NULL) {
			return "";
		}
		std::string buff;
		const int len = length;
		for (int j = 0; j < len; j++) {
			int high = bytes[j] / 16, low = bytes[j] % 16;
			buff += (high < 10) ? ('0' + high) : ('a' + high - 10);
			buff += (low < 10) ? ('0' + low) : ('a' + low - 10);
			buff += " ";
		}
		return buff;
	}

	uint32_t get_pid_by_name(const wchar_t* name)
	{
		HANDLE  hsnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hsnapshot == INVALID_HANDLE_VALUE)
		{
			return 0;
		}
		PROCESSENTRY32W pe;
		pe.dwSize = sizeof(PROCESSENTRY32W);
		BOOL find = Process32FirstW(hsnapshot, &pe);
		while (find != 0)
		{
			if (wcscmp(pe.szExeFile, name) == 0)
			{
				return pe.th32ProcessID;
			}
			find = Process32NextW(hsnapshot, &pe);
		}
		CloseHandle(hsnapshot);
		return 0;
	}

	bool kill_process_by_name(const wchar_t* name)
	{
		uint32_t pid = utils::get_pid_by_name(name);
		if (pid > 0)
		{
			HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
			if (process == NULL)
			{
				// 处理打开进程失败的情况
				return false;
			}

			bool result = TerminateProcess(process, 0);
			CloseHandle(process);
			return result;
		}
		return false;
	}

	bool create_process_by_filename(const char* filename)
	{
		STARTUPINFOA startupInfo;
		PROCESS_INFORMATION processInfo;
		ZeroMemory(&startupInfo, sizeof(startupInfo));
		ZeroMemory(&processInfo, sizeof(processInfo));

		// 启动进程
		if (!CreateProcessA(NULL, const_cast<LPSTR>(filename), NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo))
		{
			// 处理启动进程失败的情况
			return false;
		}

		// 关闭进程和线程句柄
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}

	void copy_dir(std::string dest, std::string source)
	{
		std::string command = "xcopy \"" + source + "\" \"" + dest + "\" /E /H /C /I";
		system(command.c_str());
	}

	std::string get_local_time()
	{
		time_t now = time(nullptr);
		tm timeinfo{};
		localtime_s(&timeinfo, &now);

		char buffer[80];
		strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", &timeinfo);
		return buffer;
	}

	std::string get_process_runtime_by_pid(uint32_t pid)
	{
		HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

		if (process != NULL) {
			FILETIME ftCreation, ftExit, ftKernel, ftUser;
			SYSTEMTIME stCreation, lstCreation, currentTime;

			if (GetProcessTimes(process, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
				FileTimeToSystemTime(&ftCreation, &stCreation);
				SystemTimeToTzSpecificLocalTime(NULL, &stCreation, &lstCreation);

				GetSystemTime(&currentTime);

				FILETIME ftCurrentTime;
				SystemTimeToFileTime(&currentTime, &ftCurrentTime);

				ULARGE_INTEGER startTime, currentTimeValue;
				startTime.LowPart = ftCreation.dwLowDateTime;
				startTime.HighPart = ftCreation.dwHighDateTime;

				currentTimeValue.LowPart = ftCurrentTime.dwLowDateTime;
				currentTimeValue.HighPart = ftCurrentTime.dwHighDateTime;

				ULONGLONG elapsedTime = currentTimeValue.QuadPart - startTime.QuadPart;
				elapsedTime /= 10000; // Convert to milliseconds

				// Calculate hours, minutes, and seconds
				int hours = static_cast<int>(elapsedTime / 3600000);
				int minutes = static_cast<int>((elapsedTime % 3600000) / 60000);
				int seconds = static_cast<int>((elapsedTime % 60000) / 1000);

				CloseHandle(process);

				return std::to_string(hours) + " h " + std::to_string(minutes) + " min " + std::to_string(seconds) + " sec ";
			}
			else {
				CloseHandle(process);
			}
		}
		return "no running";
	}
}
