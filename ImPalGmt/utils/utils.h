#pragma once
#include <string>
#include <stdint.h>


namespace  utils
{
	uint32_t hex2dec(std::string hex);

	// 12 -> C
	std::string dec2hex(uint64_t dec);

	// 12 -> 0000000C
	std::string dec2hex2(uint64_t dec);

	std::string utf2gbk(const char* utf8);

	std::string gbk2utf8(std::string gbkStr);

	std::string wstring2stirng(std::wstring wstr);

	std::wstring string2wstirng(std::string str);

	std::string bytesToHexString(const uint8_t* bytes, const int length);

	uint32_t get_pid_by_name(const wchar_t* name);

	bool kill_process_by_name(const wchar_t* name);

	bool create_process_by_filename(const char* filename);

	void copy_dir(std::string dest, std::string source);

	std::string get_local_time();

	std::string get_process_runtime_by_pid(uint32_t pid);
}
