#include <Windows.h>
#include <thread>
#include <chrono>

#include "hooks.hpp"
#include "menu.hpp"
#include "console.hpp"
#include "engine.hpp"
#include "config.hpp"

DWORD WINAPI initialize(void* instance)
{
	while (!GetModuleHandle(L"gameoverlayrenderer64.dll") && !GetModuleHandle(L"dxgi.dll"))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

#ifdef _DEBUG
	console::initialize(L"icarus debug");
#endif 

	console::log(L"[!] initializing\n");
	if (!engine::initialize() || !hooks::initialize() || !menu::update_configs())
	{
		MessageBox(nullptr, L"failed to initialize!", L"icarus error", MB_OK | MB_ICONERROR);
		FreeLibraryAndExitThread(static_cast<HMODULE>(instance), 0);
		return FALSE;
	}

	while (!GetAsyncKeyState(VK_END))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	FreeLibraryAndExitThread(static_cast<HMODULE>(instance), 0);

	return TRUE;
}

DWORD WINAPI release()
{
	hooks::release();

#ifdef _DEBUG
	console::release();
#endif 

	return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD call_reason, void* reserved)
{
	switch (call_reason)
	{
		case DLL_PROCESS_ATTACH:
			if (HANDLE handle = CreateThread(nullptr, 0, initialize, instance, 0, nullptr))
			{
				CloseHandle(handle);
			}
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			release();
			break;

		default:
			break;
	}

	return TRUE;
}