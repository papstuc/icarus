#include <Windows.h>

#include "registry.hpp"

bool registry::query_value(const wchar_t* path, const wchar_t* value_name, wchar_t* output, const std::uint64_t size)
{
	if (!path || !value_name || !output)
	{
		return false;
	}

	HKEY key = nullptr;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
	{
		RegCloseKey(key);
		return false;
	}

	std::uint64_t length = size;
	if (RegGetValue(key, nullptr, value_name, RRF_RT_ANY, nullptr, output, reinterpret_cast<DWORD*>(&length)) != ERROR_SUCCESS)
	{
		RegCloseKey(key);
		return false;
	}

	return true;
}
