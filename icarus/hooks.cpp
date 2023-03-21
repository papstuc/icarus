#include <Windows.h>
#include <d3d11.h>
#include <cstdint>
#include <dxgi.h>

#include "minhook/MinHook.h"

#include "hooks.hpp"

#include "console.hpp"
#include "utilities.hpp"
#include "menu.hpp"
#include "config.hpp"

static hooks::swap_chain_present::function_t swap_chain_present_original = nullptr;
static hooks::swap_chain_resize_buffers::function_t swap_chain_resize_buffers_original = nullptr;
static hooks::window_procedure::function_t window_procedure_original = nullptr;
static hooks::set_cursor::function_t set_cursor_original = nullptr;
static hooks::set_cursor_pos::function_t set_cursor_pos_original = nullptr;

static ID3D11Device* device = nullptr;
static ID3D11DeviceContext* context = nullptr;
static HWND window = nullptr;
static ID3D11RenderTargetView* render_view = nullptr;

bool hooks::initialize()
{
	void* swap_chain_present_target = static_cast<void*>(utilities::pattern_scan(L"dxgi.dll", "E9 ? ? ? ? 48 89 74 24 ? 55"));
	void* swap_chain_resize_buffers_target = static_cast<void*>(utilities::pattern_scan(L"dxgi.dll", "E9 ? ? ? ? 54 41 55 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 48 C7 45 ? ? ? ? ? 48 89 58 ? 48 89 70 ? 48 89 78 ? 45 8B F9 45 8B E0 44 8B EA 48 8B F9 8B 45 ? 89 44 24 ? 8B 75 ? 89 74 24 ? 44 89 4C 24 ? 45 8B C8 44 8B C2 48 8B D1 48 8D 4D ? E8 ? ? ? ? 48 8B C7 48 8D 4F ? 48 F7 D8 48 1B DB 48 23 D9 48 8B 4B ? 48 8B 01 48 8B 40 ? FF 15 ? ? ? ? 48 89 5D ? 8B 87 ? ? ? ? 89 45 ? 45 33 F6"));

	if (MH_Initialize() != MH_OK)
	{
		console::log(L"[-] failed to initialize minhook\n");
		return false;
	}

	if (MH_CreateHook(swap_chain_present_target, &hooks::swap_chain_present::hook, reinterpret_cast<void**>(&swap_chain_present_original)) != MH_OK)
	{
		console::log(L"[-] failed to hook CDXGISwapChain!Present\n");
		return false;
	}

	if (MH_CreateHook(swap_chain_resize_buffers_target, &hooks::swap_chain_resize_buffers::hook, reinterpret_cast<void**>(&swap_chain_resize_buffers_original)) != MH_OK)
	{
		console::log(L"[-] failed to hook CDXGISwapChain!ResizeBuffers\n");
		return false;
	}

	if (MH_CreateHook(&SetCursor, &hooks::set_cursor::hook, reinterpret_cast<void**>(&set_cursor_original)) != MH_OK)
	{
		console::log(L"[-] failed to hook SetCursor\n");
		return false;
	}

	if (MH_CreateHook(&SetCursorPos, &hooks::set_cursor_pos::hook, reinterpret_cast<void**>(&set_cursor_pos_original)) != MH_OK)
	{
		console::log(L"[-] failed to hook SetCursorPos\n");
		return false;
	}

	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
	{
		console::log(L"[-] failed to enable hooks\n");
		return false;
	}

	console::log(L"[+] hooks initialized\n");

	return true;
}

void hooks::release()
{
	SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<std::int64_t>(window_procedure_original));

	MH_Uninitialize();
	MH_DisableHook(MH_ALL_HOOKS);
}

HRESULT __fastcall hooks::swap_chain_present::hook(IDXGISwapChain* swap_chain, std::uint32_t sync_interval, std::uint32_t flags)
{
	if (!device)
	{
		ID3D11Texture2D* buffer;

		swap_chain->GetBuffer(0, IID_PPV_ARGS(&buffer));

		if (buffer)
		{
			swap_chain->GetDevice(IID_PPV_ARGS(&device));
			device->CreateRenderTargetView(buffer, nullptr, &render_view);
			device->GetImmediateContext(&context);
		}

		DXGI_SWAP_CHAIN_DESC desc;
		swap_chain->GetDesc(&desc);
		window = desc.OutputWindow;

		if (window_procedure_original && window)
		{
			SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(window_procedure_original));
			window_procedure_original = nullptr;
		}

		if (window)
		{
			window_procedure_original = reinterpret_cast<decltype(window_procedure_original)>(SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hooks::window_procedure::hook)));
		}

		menu::initialize(window, device, context);
	}

	menu::render(context, render_view);

	return swap_chain_present_original(swap_chain, sync_interval, flags);
}

HRESULT __fastcall hooks::swap_chain_resize_buffers::hook(IDXGISwapChain* swap_chain, std::uint32_t buffer_count, std::uint32_t width, std::uint32_t height, DXGI_FORMAT new_format, std::uint32_t swap_chain_flags)
{
	if (render_view)
	{
		render_view->Release();
		render_view = nullptr;
	}

	if (context)
	{
		context->Release();
		context = nullptr;
	}

	if (device)
	{
		device->Release();
		device = nullptr;
	}

	menu::release();

	return swap_chain_resize_buffers_original(swap_chain, buffer_count, width, height, new_format, swap_chain_flags);
}

LRESULT __stdcall hooks::window_procedure::hook(HWND window, std::uint32_t message, WPARAM wparam, LPARAM lparam)
{
	if (message == WM_KEYDOWN && LOWORD(wparam) == VK_INSERT)
	{
		config::menu_open = !config::menu_open;
	}

	if (menu::handle_window_procedure(window, message, wparam, lparam) && config::menu_open)
	{
		return true;
	}

	if (config::menu_open)
	{
		std::int32_t imgui_cursor = menu::get_mouse_cursor();
		LPWSTR win32_cursor = IDC_ARROW;

		switch (imgui_cursor)
		{
			case ImGuiMouseCursor_Arrow:
				win32_cursor = IDC_ARROW;
				break;
			case ImGuiMouseCursor_TextInput:
				win32_cursor = IDC_IBEAM;
				break;
			case ImGuiMouseCursor_ResizeAll:
				win32_cursor = IDC_SIZEALL;
				break;
			case ImGuiMouseCursor_ResizeEW:
				win32_cursor = IDC_SIZEWE;
				break;
			case ImGuiMouseCursor_ResizeNS:
				win32_cursor = IDC_SIZENS;
				break;
			case ImGuiMouseCursor_ResizeNESW:
				win32_cursor = IDC_SIZENESW;
				break;
			case ImGuiMouseCursor_ResizeNWSE:
				win32_cursor = IDC_SIZENWSE;
				break;
			case ImGuiMouseCursor_Hand:
				win32_cursor = IDC_HAND;
				break;
			case ImGuiMouseCursor_NotAllowed:
				win32_cursor = IDC_NO;
				break;
		}
		set_cursor_original(LoadCursor(nullptr, win32_cursor));
	}
	else
	{
		return CallWindowProc(window_procedure_original, window, message, wparam, lparam);
	}

	return DefWindowProc(window, message, wparam, lparam);
}

HCURSOR __stdcall hooks::set_cursor::hook(HCURSOR cursor)
{
	return set_cursor_original(cursor);
}

BOOL __stdcall hooks::set_cursor_pos::hook(std::int32_t x, std::int32_t y)
{
	return set_cursor_pos_original(x, y);
}
