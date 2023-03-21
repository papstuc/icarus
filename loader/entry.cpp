#include <filesystem>
#include <Windows.h>
#include <thread>
#include <chrono>
#include <cstdio>

#include "registry.hpp"
#include "process.hpp"

static bool choose_file(wchar_t* output, std::uint64_t size)
{
	if (!output)
	{
		return false;
	}

	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GetConsoleWindow();
	ofn.lpstrFile = output;
	ofn.nMaxFile = static_cast<DWORD>(size);
	ofn.lpstrFilter = L"Application extension\0*.dll\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	return GetOpenFileName(&ofn);
}

int wmain(int argc, wchar_t** argv)
{
	wchar_t steam_path[MAX_PATH];
	if (registry::query_value(L"SOFTWARE\\Valve\\Steam", L"SteamExe", steam_path, sizeof(steam_path)))
	{ 
		printf("[+] steam path -> %ls\n", steam_path);
	}
	else
	{
		printf("[-] failed to find steam in the registry\n");
		return EXIT_FAILURE;
	}

	wchar_t binary_path[MAX_PATH] = { 0 };
	if (argc == 2)
	{
		if (std::filesystem::exists(std::filesystem::path(argv[1])))
		{
			printf("[+] argument-specified file found\n");
			wcscpy_s(binary_path, argv[1]);
		}
		else
		{
			printf("[-] failed to find argument-specified file, opening file chooser\n");
			if (choose_file(binary_path, sizeof(binary_path)))
			{
				printf("[+] found file\n");
			}
			else
			{
				printf("[-] failed to choose file\n");
				return EXIT_FAILURE;
			}
		}
	}
	else
	{
		printf("[+] no arguments specified, opening file chooser\n");
		if (choose_file(binary_path, sizeof(binary_path)))
		{
			printf("[+] found file\n");
		}
		else
		{
			printf("[-] failed to choose file\n");
			return EXIT_FAILURE;
		}
	}

	std::uint64_t process_id = process::find_process_id(L"SoTGame.exe");

	if (!process_id)
	{
		if (!process::start_process(steam_path, L"steam://rungameid/1172620"))
		{
			printf("[-] failed to start process\n");
			return EXIT_FAILURE;
		}

		printf("[+] waiting for sea of thieves\n");

		while (process_id = process::find_process_id(L"SoTGame.exe"), process_id == 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
		}
	}

	if (process::inject_dll(process_id, binary_path, sizeof(binary_path)))
	{
		printf("[+] binary injected\n");
	}
	else
	{
		printf("[-] failed to inject binary\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}