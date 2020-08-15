#ifndef MEMORY_HELPERS_HPP__
#define MEMORY_HELPERS_HPP__

#include <array>
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
        return WriteMemory(address, &bytes[0], N, rebase);
    }

    template <size_t N>
    void WriteMemory(uintptr_t address, std::array<uint8_t, N> const& bytes, bool rebase = true)
    {
        return WriteMemory(address, bytes.data(), N, rebase);
    }

    void WriteMemory(uintptr_t address, const uint8_t* data, size_t size, bool rebase = true);

    void FlushInstructionCache(uintptr_t address, size_t N, bool rebase = true);
}

#endif // MEMORY_HELPERS_HPP__