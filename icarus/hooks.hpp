#pragma once

#include <cstdint>
#include <dxgi.h>

namespace hooks
{
	bool initialize();
	void release();

	namespace swap_chain_present
	{
		using function_t = HRESULT(__fastcall*)(IDXGISwapChain*, std::uint32_t, std::uint32_t);
		HRESULT __fastcall hook(IDXGISwapChain* swap_chain, std::uint32_t sync_interval, std::uint32_t flags);
	}

	namespace swap_chain_resize_buffers
	{
		using function_t = HRESULT(__fastcall*)(IDXGISwapChain*, std::uint32_t, std::uint32_t, std::uint32_t, DXGI_FORMAT, std::uint32_t);
		HRESULT __fastcall hook(IDXGISwapChain* swap_chain, std::uint32_t buffer_count, std::uint32_t width, std::uint32_t height, DXGI_FORMAT new_format, std::uint32_t swap_chain_flags);
	}

	namespace window_procedure
	{
		using function_t = LRESULT(__stdcall*)(HWND, std::uint32_t, WPARAM, LPARAM);
		LRESULT __stdcall hook(HWND window, std::uint32_t message, WPARAM wparam, LPARAM lparam);
	}

	namespace set_cursor
	{
		using function_t = HCURSOR(__stdcall*)(HCURSOR);
		HCURSOR __stdcall hook(HCURSOR cursor);
	}

	namespace set_cursor_pos
	{
		using function_t = BOOL(__stdcall*)(std::int32_t x, std::int32_t y);
		BOOL __stdcall hook(std::int32_t x, std::int32_t y);
	}
}