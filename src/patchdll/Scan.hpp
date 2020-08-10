#ifndef MEMORY_SCAN_H_
#define MEMORY_SCAN_H_

#include <cstdint>
#include <functional>
#include <vector>

#include <Windows.h>

namespace Memory {
    DWORD Protect(uintptr_t offset, DWORD pageProtection);

    namespace Scan {
        using Pattern = uint16_t;
        constexpr static const Pattern Wildcard = 0xFFFF;

        constexpr static const uintptr_t NotFound = std::numeric_limits<uintptr_t>::infinity();

        template <std::size_t N>
        constexpr uintptr_t FindFirst(Pattern const(&pattern)[N], uintptr_t startOffset) {
            // Not exactly efficient, but whatever
            auto address = Find(pattern, startOffset);
            if (address.empty())
                return NotFound;

            return address.front();

        }

        template <std::size_t N>
        constexpr std::vector<uintptr_t> Find(Pattern const(&pattern)[N], uintptr_t startOffset) {
            std::vector<uintptr_t> addresses;

            uintptr_t currentAddr = uintptr_t(GetModuleHandle(nullptr));
            MEMORY_BASIC_INFORMATION mbi;

            uint32_t patternIndex = 0;

            for (; VirtualQuery(reinterpret_cast<LPVOID>(currentAddr), &mbi, sizeof(mbi)) == sizeof(mbi); currentAddr += mbi.RegionSize) {
                DWORD oldProtection = 0;

                if (!VirtualProtect(reinterpret_cast<LPVOID>(currentAddr), mbi.RegionSize, PAGE_EXECUTE_READWRITE, &oldProtection))
                {
                    // ???
                    if (oldProtection & PAGE_NOACCESS)
                        continue;
                }
                
                // Abuse RAII in case somehow we read invalid memory (that shouldn't happen...)
                std::shared_ptr<void> protectRestoreHandle(nullptr,
                    [&oldProtection, &mbi, &currentAddr](void*) {
                        VirtualProtect(reinterpret_cast<LPVOID>(currentAddr), mbi.RegionSize, oldProtection, &oldProtection);
                    });

                for (uintptr_t x = currentAddr; x < (currentAddr + mbi.RegionSize); x++)
                {
                    uint16_t patternMask = pattern[patternIndex];

                    if (patternMask == Wildcard || *reinterpret_cast<uint8_t*>(x) == (patternMask & 0xFF)) {
                        patternIndex++;

                        if (patternIndex >= N)
                            addresses.push_back(x - N + 1);
                    }
                    else
                        patternIndex = 0;

                }
            }

            return addresses;
        }
    }
}

#endif // MEMORY_SCAN_H_
