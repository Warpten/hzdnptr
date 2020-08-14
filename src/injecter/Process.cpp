// By Shauren, with additions and adjustments by me.

#include "Process.hpp"

#include <cstdio>
#include <filesystem>
#include <psapi.h>
#include <TlHelp32.h>

#pragma comment(lib, "Version.lib")
#pragma comment(lib, "Psapi.lib")

template<>
std::string const& Process::Read<std::string>(std::uintptr_t address, bool relative /*= true*/)
{
    if (relative)
        address += _baseAddress;

    if (std::string const* str = _stringCache.Retrieve<std::string>(address))
        return *str;

    char buffer;
    std::string uncached;
    std::uint8_t const* addr = reinterpret_cast<std::uint8_t const*>(address);
    while (ReadProcessMemory(_handle, addr++, &buffer, sizeof(buffer), nullptr) && buffer != '\0')
        uncached.append(1, buffer);

    return _stringCache.Store(address, uncached);
}

std::vector<Process::Module> Process::EnumerateModules() const
{
    std::vector<Process::Module> modules;

    HMODULE moduleHandles[512];
    DWORD cbNeeded = 0;
    if (EnumProcessModules(_handle, moduleHandles, sizeof(moduleHandles), &cbNeeded))
    {
        for (uint32_t i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            TCHAR szModName[MAX_PATH];

            // Get the full path to the module's file.

            if (GetModuleFileNameEx(_handle, moduleHandles[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
            {
                modules.emplace_back(moduleHandles[i], std::filesystem::path(szModName).filename().string());
            }
        }
    }

    return modules;
}

std::shared_ptr<Process> ProcessTools::Open(std::string const name, bool log)
{
    DWORD_PTR baseAddress;
    HANDLE hnd = GetHandleByName((TCHAR*) name.c_str(), &baseAddress, log);
    if (hnd == INVALID_HANDLE_VALUE)
        return std::shared_ptr<Process>();

    return std::make_shared<Process>(hnd, baseAddress);
}

HANDLE ProcessTools::GetHandleByName(TCHAR* name, DWORD_PTR* baseAddress, bool log)
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (!Process32First(snapshot, &entry))
    {
        CloseHandle(snapshot);
        if (log)
            _tprintf(_T("Cannot find any process in system!\n"));
        return INVALID_HANDLE_VALUE;
    }

    HANDLE process = INVALID_HANDLE_VALUE;
    do
    {
        if (!_tcsicmp(entry.szExeFile, name))
        {
            process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
            if (process)
            {
                if (baseAddress)
                {
                    MODULEENTRY32 module;
                    module.dwSize = sizeof(MODULEENTRY32);

                    HANDLE moduleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, entry.th32ProcessID);
                    if (Module32First(moduleSnapshot, &module))
                    {
                        if (baseAddress)
                            *baseAddress = (DWORD_PTR)module.modBaseAddr;
                    }

                    CloseHandle(moduleSnapshot);
                }

                break;
            }
        }
    } while (Process32Next(snapshot, &entry));

    CloseHandle(snapshot);

    if (process == INVALID_HANDLE_VALUE)
    {
        if (log)
            _tprintf(_T("Process with name %s not running.\n"), name);
        return INVALID_HANDLE_VALUE;
    }

    if (!process)
    {
        if (log)
            _tprintf(_T("Process with name %s is couldn't be opened.\n"), name);
        return INVALID_HANDLE_VALUE;
    }

    return process;
}
