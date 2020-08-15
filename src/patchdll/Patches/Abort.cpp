#include "Abort.hpp"
#include "../Assembly/Decoder.hpp"
#include "../Assembly/Function.hpp"
#include "../Assembly/Instruction.hpp"

#include "../Logger.hpp"
#include "../MemoryHelpers.hpp"
#include "../Scan.hpp"

#include <windows.h>

namespace Patches
{
    namespace Abort
    {
        void Apply()
        {
            uintptr_t moduleBase = uintptr_t(GetModuleHandle(nullptr));

            auto console = Logger::Instance().GetLogger("console");

            console->info("Scanning for *(volatile int*)(0) = 0x0DEADCA7;");

            constexpr static const Memory::Scan::Pattern NullWrites[] = { 0xC7, 0x04, 0x25, 0x00, 0x00, 0x00, 0x00, 0xA7, 0xDC, 0xEA, 0x0D };
            std::vector<uintptr_t> faultyAddresses = Memory::Scan::Find(NullWrites, moduleBase);

            uint32_t failedCount = 0;
            for (uintptr_t faultyAddress : faultyAddresses)
            {
                Assembly::Function function(faultyAddress);
                std::optional<uintptr_t> epilogueAddress = function.FindEpilogue();

                if (!epilogueAddress)
                {
                    ++failedCount;
                    console->warn("  HorizonZeroDawn.exe+0x{:x}: Function epilogue not found.", faultyAddress - moduleBase);
                }
                else
                {
                    // Patch to a jmp rel32 - should be enough in 99% cases
                    uint32_t distance = uint32_t(epilogueAddress.value() - faultyAddress - 5uLL);
                    if (distance < std::numeric_limits<uint32_t>::max())
                    {
                        uint8_t patchData[] = {
                            0xE9, 0x00, 0x00, 0x00, 0x00,           // JMP rel32
                            0x90, 0x90, 0x90, 0x90, 0x90, 0x90      // A bunch of NOPs for the patch to look nice
                        };

                        memcpy(&patchData[1], &distance, sizeof(uint32_t));
                        Memory::WriteMemory(faultyAddress, patchData, false);
                        Memory::FlushInstructionCache(faultyAddress, sizeof(patchData), false);

                        console->info("  HorizonZeroDawn.exe+0x{:x}): Patched.", faultyAddress - moduleBase);
                    }
                    else
                    {
                        ++failedCount;
                        console->error("HorizonZeroDawn.exe+0x{:x}): Epilogue is too far away ({} bytes).", faultyAddress - moduleBase, std::abs(int32_t(distance)));
                    }
                }
            }

            console->info("{} of {} addresses patched.", faultyAddresses.size() - failedCount, faultyAddresses.size());
        }
    }
}
