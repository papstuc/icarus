#pragma once

#include <cstdint>

namespace process
{
	bool inject_dll(const std::uint64_t process_id, const wchar_t* file_path, const std::uint64_t size);
	bool start_process(const wchar_t* file_path, const wchar_t* arguments);
	std::uint64_t find_process_id(const wchar_t* process_name);
}