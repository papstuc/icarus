#pragma once

#include <cstdint>

namespace registry
{
	bool query_value(const wchar_t* path, const wchar_t* value_name, wchar_t* output, std::uint64_t size);
}