#ifndef MEMORY_HELPERS_HPP__
#define MEMORY_HELPERS_HPP__

#include <cstdint>
#include <string_view>

#include <windows.h>

namespace Memory
{
    uintptr_t RebaseAddress(uintptr_t address);
    std::string_view ReadString(uintptr_t address, bool rebase = true);

    template <typename T>
    T ReadMemory(uintptr_t address, bool rebase = true)
    {
        if (rebase)
            address = RebaseAddress(address);

        return *reinterpret_cast<T*>(address);
    }

    template <size_t N>
    void WriteMemory(uintptr_t address, uint8_t const (&bytes)[N], bool rebase = true)
    {
        if (rebase)
            address = RebaseAddress(address);

        if (address == 0)
            return;

        uint8_t* dst = reinterpret_cast<uint8_t*>(address);

        DWORD oldProtection;
        if (!VirtualProtect(dst, N, PAGE_EXECUTE_READWRITE, &oldProtection))
            return;

        memcpy(dst, bytes, N);
        VirtualProtect(dst, N, oldProtection, &oldProtection);
    }

    void FlushInstructionCache(uintptr_t address, size_t N, bool rebase = true);
}

#endif // MEMORY_HELPERS_HPP__