#include <vector>
#include <Windows.h>

#include "utilities.hpp"

static std::vector<std::uint32_t> pattern_to_byte(const char* pattern)
{
	std::vector<std::uint32_t> bytes;
	char* start = const_cast<char*>(pattern);
	char* end = const_cast<char*>(pattern) + std::strlen(pattern);

	for (char* current = start; current < end; current++)
	{
		if (*current == '?')
		{
			current++;

			if (*current == '?')
			{
				current++;
			}

			bytes.push_back(-1);
		}
		else
		{
			bytes.push_back(std::strtoul(current, &current, 16));
		}
	}

	return bytes;
}

std::uint8_t* utilities::pattern_scan(const wchar_t* module_name, const char* signature)
{
	const HMODULE module_handle = GetModuleHandle(module_name);

	if (!module_handle)
	{
		return nullptr;
	}

	const PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);
	const PIMAGE_NT_HEADERS64 nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module_handle) + dos_header->e_lfanew);

	const std::size_t size_of_image = nt_headers->OptionalHeader.SizeOfImage;
	const std::vector<std::uint32_t> pattern_bytes = pattern_to_byte(signature);
	const std::uint8_t* image_base = reinterpret_cast<std::uint8_t*>(module_handle);

	const std::size_t pattern_size = pattern_bytes.size();
	const std::uint32_t* array_of_bytes = pattern_bytes.data();

	for (std::size_t i = 0; i < size_of_image - pattern_size; i++)
	{
		bool found = true;

		for (std::size_t j = 0; j < pattern_size; j++)
		{
			if (image_base[i + j] != array_of_bytes[j] && array_of_bytes[j] != -1)
			{
				found = false;
				break;
			}
		}

		if (found)
		{
			return &const_cast<std::uint8_t*>(image_base)[i];
		}
	}

	return nullptr;
}

std::uint8_t* utilities::resolve_rip(const std::uint8_t* address, const std::uint32_t rva_offset, const std::uint32_t rip_offset)
{
	if (!address || !rva_offset || !rip_offset)
	{
		return nullptr;
	}

	std::uint32_t rva = *reinterpret_cast<std::uint32_t*>(const_cast<std::uint8_t*>(address) + rva_offset);
	std::uint64_t rip = reinterpret_cast<std::uint64_t>(address) + rip_offset;

	return reinterpret_cast<std::uint8_t*>(rva + rip);
}
