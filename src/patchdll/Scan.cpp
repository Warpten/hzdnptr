#include "Scan.hpp"

namespace Memory
{
    DWORD Protect(uintptr_t offset, DWORD pageProtection) {
        MEMORY_BASIC_INFORMATION mbi;
        VirtualQuery(reinterpret_cast<LPVOID>(offset), &mbi, sizeof(mbi));

        DWORD oldProtection;
        VirtualProtect(static_cast<LPVOID>(mbi.BaseAddress), mbi.RegionSize, pageProtection, &oldProtection);
        return oldProtection;
    }
}
