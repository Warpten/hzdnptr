// By Shauren, with some additions and adjustments by me.

#ifndef ProcessTools_h__
#define ProcessTools_h__

#include "Cache.hpp"

#include <Windows.h>
#include <tchar.h>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

struct FileVersionInfo
{
    WORD FileMajorPart;
    WORD FileMinorPart;
    WORD FileBuildPart;
    WORD FilePrivatePart;

    void Init(DWORD ms, DWORD ls)
    {
        FileMajorPart = HIWORD(ms);
        FileMinorPart = LOWORD(ms);
        FileBuildPart = HIWORD(ls);
        FilePrivatePart = LOWORD(ls);
    }
};

struct Process
{
    struct Module
    {
        HMODULE Base;
        std::string Name;

        Module(HMODULE base, std::string name) : Base(base), Name(name) { }
    };

    Process(HANDLE hnd, std::uintptr_t baseAddress, FileVersionInfo const& ver) : _handle(hnd), _baseAddress(baseAddress), _version(ver) { }
    ~Process()
    {
        CloseHandle(_handle);
    }

    template <typename T>
    T const& Read(std::uintptr_t address, bool relative = true)
    {
        if (relative)
            address += _baseAddress;

        if (T const* value = _dataCache.Retrieve<T>(address))
            return *value;

        T uncached;
        ReadProcessMemory(_handle, reinterpret_cast<LPCVOID>(address), &uncached, sizeof(T), nullptr);
        return _dataCache.Store(address, uncached);
    }

    std::string ReadString(std::uintptr_t address, bool relative = true)
    {
        if (relative)
            address += _baseAddress;

        std::stringstream ss;
        char last;
        do {
            last = Read<char>(address++, false);
            if (last != '\0')
                ss << last;
        } while (last != '\0');

        return ss.str();
    }

    template<typename T>
    std::vector<T> ReadArray(std::uintptr_t address, std::size_t arraySize, bool relative = true) const
    {
        if (relative)
            address += _baseAddress;

        std::vector<T> data(arraySize);
        ReadProcessMemory(_handle, reinterpret_cast<LPCVOID>(address), data.data(), sizeof(T) * arraySize, nullptr);
        return data;
    }

    // pointer wrappers
    template<typename T>
    T const& Read(void const* address) { return Read<T>(reinterpret_cast<std::uintptr_t>(address), false); }

    template<typename T>
    std::vector<T> ReadArray(void const* address, std::size_t arraySize) { return ReadArray<T>(reinterpret_cast<std::uintptr_t>(address), arraySize, false); }

    bool IsValidAddress(void const* address)
    {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQueryEx(_handle, address, &mbi, sizeof(mbi)))
            return false;

        return mbi.Protect != PAGE_NOACCESS;
    }

    FileVersionInfo const& GetFileVersionInfo() const { return _version; }

    HANDLE GetHandle() const { return _handle; }

    std::uintptr_t AllocateMemory(size_t size)
    {
        return (std::uintptr_t) VirtualAllocEx(_handle, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    }

    void FreeMemory(std::uintptr_t address, size_t size, bool relative = false)
    {
        if (relative)
            address += _baseAddress;

        VirtualFreeEx(_handle, LPVOID(address), 0, MEM_RELEASE);
    }

    bool Write(std::uintptr_t address, LPCVOID data, size_t dataSize, bool relative = true)
    {
        if (relative)
            address += _baseAddress;

        // Make memory writable.
        DWORD oldProtection = Protect(address, dataSize, PAGE_READWRITE, false);
        std::shared_ptr<void> cleanup(nullptr, [&](void*) { Protect(address, dataSize, oldProtection, false); });

        SIZE_T bytesWritten = 0;
        if (!WriteProcessMemory(_handle, LPVOID(address), data, dataSize, &bytesWritten))
            return false;

        return true;
    }

    bool WriteCString(std::uintptr_t address, std::string const& string, bool relative = true)
    {
        if (relative)
            address += _baseAddress;

        size_t size = string.length() + 1;

        DWORD oldProtection = Protect(address, size, PAGE_READWRITE, false);
        std::shared_ptr<void> cleanup(nullptr, [&](void*) { Protect(address, size, oldProtection, false); });

        SIZE_T bytesWritten = 0;
        if (!WriteProcessMemory(_handle, LPVOID(address), string.data(), string.length(), &bytesWritten))
            return false;

        address += string.length();

        static char nullTerminator = '\0';
        if (!WriteProcessMemory(_handle, LPVOID(address), &nullTerminator, 1, &bytesWritten))
            return false;

        return true;
        
    }

    DWORD Protect(std::uintptr_t address, size_t size, DWORD protection, bool relative = true)
    {
        if (relative)
            address += _baseAddress;

        VirtualProtectEx(_handle, LPVOID(address), size, protection, &protection);
        return protection;
    }

    template <typename T = void>
    T CreateRemoteThread(std::uintptr_t fptr, std::uintptr_t args, bool relative = false)
    {
        if (relative)
            fptr += _baseAddress;

        HANDLE tHandle = ::CreateRemoteThread(_handle, nullptr, 0, LPTHREAD_START_ROUTINE(fptr), LPVOID(args), 0, nullptr);
        if (tHandle != nullptr)
        {
            WaitForSingleObject(tHandle, INFINITE);

            DWORD resultCode;
            while (GetExitCodeThread(tHandle, &resultCode))
                if (resultCode == STILL_ACTIVE)
                    Sleep(50);
                else
                    break;

            CloseHandle(tHandle);

            if constexpr (sizeof(T) == sizeof(DWORD))
                return *reinterpret_cast<T*>(&resultCode);
            else
            {

            }
        }

        if constexpr (!std::is_same_v<T, void>)
            return T{};
    }

    std::vector<Module> EnumerateModules() const;

private:
    HANDLE _handle;
    std::uintptr_t _baseAddress;
    FileVersionInfo _version;
    Cache<256> _dataCache;
    Cache<1024> _stringCache;
};

template<>
std::string const& Process::Read<std::string>(std::uintptr_t address, bool relative);

namespace ProcessTools
{
    std::shared_ptr<Process> Open(std::string const name, DWORD build, bool log);

    HANDLE GetHandleByName(TCHAR* name, DWORD_PTR* baseAddress, DWORD build, bool log, FileVersionInfo* versionInfo);
    void GetFileVersion(TCHAR* path, FileVersionInfo* info);
}

#endif // ProcessTools_h__