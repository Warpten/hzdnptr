#include "MemoryHelpers.hpp"

#include <windows.h>

namespace Memory
{
    uintptr_t RebaseAddress(uintptr_t address)
    {
        return address + uintptr_t(GetModuleHandle(nullptr)) - 0x0000000140000000uLL;
    }

    std::string_view ReadString(uintptr_t address, bool rebase /* = true */)
    {
        return ReadMemory<const char*>(address, rebase);
    }

    void FlushInstructionCache(uintptr_t address, size_t N, bool rebase /* = true */)
    {
        if (rebase)
            address = RebaseAddress(address);

        if (address == 0)
            return;

        ::FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<uint8_t*>(address), N);
    }

    void WriteMemory(uintptr_t address, const uint8_t* data, size_t size, bool rebase /* = true */)
    {
        if (rebase)
            address = RebaseAddress(address);

        if (address == 0)
            return;

        uint8_t* dst = reinterpret_cast<uint8_t*>(address);

        DWORD oldProtection;
        if (!VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtection))
            return;

        memcpy(dst, data, size);
        VirtualProtect(dst, size, oldProtection, &oldProtection);
    }

}
