#ifndef Module_hpp__
#define Module_hpp__

#include <Windows.h>

#include <cstdint>
#include <string_view>

struct Library
{
    Library(const std::string_view moduleName)
    {
        _handle = LoadLibraryA(moduleName.data());
    }
    
    ~Library()
    {
        if (_handle != nullptr)
            FreeLibrary(_handle);
    }

    std::uintptr_t GetProcOffset(const std::string_view procName, bool relative = true)
    {
        if (_handle == nullptr)
            return 0u;

        std::uintptr_t procAddress = std::uintptr_t(GetProcAddress(_handle, procName.data()));
        if (procAddress != 0u && relative)
            procAddress -= std::uintptr_t(_handle);

        return procAddress;
    }

    std::uintptr_t GetBaseAddress() const { return std::uintptr_t(_handle); }

private:
    HMODULE _handle;
};

#endif // Module_hpp__
