#include <Windows.h>
#include <iostream>
#include <cstdint>

#include "ntapi.hpp"
#include "process.hpp"

bool process::inject_dll(std::uint64_t process_id, const wchar_t* file_path, std::uint64_t size)
{
    if (!process_id || !file_path || !size)
    {
        return false;
    }

	HANDLE remote_process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, static_cast<DWORD>(process_id));

    if (!remote_process)
    {
        return false;
    }

    void* remote_memory = VirtualAllocEx(remote_process, nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (!remote_memory)
    {
        CloseHandle(remote_process);
        return false;
    }

    if (!WriteProcessMemory(remote_process, remote_memory, file_path, size, nullptr))
    {
        CloseHandle(remote_process);
        return false;
    }

    HANDLE thread = CreateRemoteThread(remote_process, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibrary), remote_memory, 0, nullptr);
    
    if (!thread)
    {
        CloseHandle(remote_process);
        return false;
    }

    CloseHandle(thread);
    CloseHandle(remote_process);

	return true;
}

bool process::start_process(const wchar_t* file_path, const wchar_t* arguments)
{
	HWND shell = GetShellWindow();

    DWORD pid;
	GetWindowThreadProcessId(shell, &pid);

    HANDLE process = OpenProcess(PROCESS_CREATE_PROCESS, FALSE, pid);

    std::uint64_t buffer_size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &buffer_size);

    PPROC_THREAD_ATTRIBUTE_LIST thread_list = static_cast<PPROC_THREAD_ATTRIBUTE_LIST>(VirtualAlloc(nullptr, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

    if (!thread_list)
    {
        return false;
    }

    if (!InitializeProcThreadAttributeList(thread_list, 1, 0, &buffer_size))
    {
        VirtualFree(thread_list, 0, MEM_RELEASE);
        return false;
    }

    if (!UpdateProcThreadAttribute(thread_list, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &process, sizeof(process), nullptr, nullptr))
    {
        VirtualFree(thread_list, 0, MEM_RELEASE);
        return false;
    }

    STARTUPINFOEX siex = { };
    siex.lpAttributeList = thread_list;
    siex.StartupInfo.cb = sizeof(siex);
    PROCESS_INFORMATION pi = { };

    std::wstring process_ctx = std::wstring(L"\"" + std::wstring(file_path) + L"\" " + std::wstring(arguments));
    if (!CreateProcess(nullptr, const_cast<wchar_t*>(process_ctx.c_str()), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE | EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr, &siex.StartupInfo, &pi))
    {
        VirtualFree(thread_list, 0, MEM_RELEASE);
        return false;
    }

    if (!CloseHandle(pi.hProcess) || !CloseHandle(pi.hThread) || !CloseHandle(process))
    {
        VirtualFree(thread_list, 0, MEM_RELEASE);
        return false;
    }

    VirtualFree(thread_list, 0, MEM_RELEASE);
	return true;
}

std::uint64_t process::find_process_id(const wchar_t* process_name)
{
    std::uint64_t size = 0;
    NTSTATUS status = NtQuerySystemInformation(SystemProcessInformation, nullptr, 0, reinterpret_cast<DWORD*>(&size));

    if (!size)
    {
        return 0;
    }

    const std::uint64_t buffer_size = size;
    PSYSTEM_PROCESS_INFORMATION system_information = static_cast<PSYSTEM_PROCESS_INFORMATION>(VirtualAlloc(nullptr, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

    if (!system_information)
    {
        return 0;
    }

    if (NT_SUCCESS(NtQuerySystemInformation(SystemProcessInformation, system_information, static_cast<unsigned long>(buffer_size), reinterpret_cast<DWORD*>(&size))))
    {
        while (system_information->NextEntryOffset)
        {
            UNICODE_STRING unicode_string = { 0 };
            RtlInitUnicodeString(&unicode_string, process_name);

            if (RtlEqualUnicodeString(&unicode_string, &system_information->ImageName, TRUE))
            {
                const std::uint64_t process_id = reinterpret_cast<std::uint64_t>(system_information->UniqueProcessId);

                VirtualFree(system_information, 0, MEM_RELEASE);
                return process_id;
            }

            system_information = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(reinterpret_cast<std::uintptr_t>(system_information) + system_information->NextEntryOffset);
        }
    }

    VirtualFree(system_information, 0, MEM_RELEASE);

    return 0;
}
