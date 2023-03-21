#pragma once

#include <cstdint>

namespace utilities
{
	std::uint8_t* pattern_scan(const wchar_t* module_name, const char* signature);
	std::uint8_t* resolve_rip(std::uint8_t* address, std::uint32_t rva_offset, std::uint32_t rip_offset);

	inline std::uint32_t color_to_value(float* color)
	{
		std::uint32_t out = 0;

		out = static_cast<std::uint32_t>(color[0] * 255.f) << 0;
		out |= static_cast<std::uint32_t>(color[1] * 255.f) << 8;
		out |= static_cast<std::uint32_t>(color[2] * 255.f) << 16;
		out |= static_cast<std::uint32_t>(color[3] * 255.f) << 24;

		return out;
	}
}
