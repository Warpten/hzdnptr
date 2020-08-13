#include "DebugLogPatch.hpp"
#include "../Assembly/Decoder.hpp"
#include "../Assembly/Function.hpp"
#include "../Assembly/Instruction.hpp"

#include "../Logger.hpp"
#include "../MemoryHelpers.hpp"
#include "../Scan.hpp"

#include <windows.h>

namespace Patches
{
    namespace DebugLogs
    {
        constexpr static const Memory::Scan::Pattern NullWrites[] = { 0xC7, 0x04, 0x25, 0x00, 0x00, 0x00, 0x00, 0xA7, 0xDC, 0xEA, 0x0D };

        LONG WINAPI UnhandledExceptionFilter(_EXCEPTION_POINTERS* exceptionInfo)
        {
            if (exceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
            {
                // Invalid memory write
                if (exceptionInfo->ExceptionRecord->ExceptionInformation[0] == 1)
                {
                    Logger::Instance().GetLogger("console")->info("Intercepted invalid memory write to 0x{:X}", exceptionInfo->ExceptionRecord->ExceptionInformation[1]);
                }

                // Silence the error
                // TODO: temporary band-aid until proper analysis of crash at 7B60A4
                return EXCEPTION_CONTINUE_EXECUTION;
            }

            // Keep searching for a handler
            return EXCEPTION_CONTINUE_SEARCH;
        }

        void ApplyPatches()
        {
            auto console = Logger::Instance().GetLogger("console");

            console->info("Scanning executable code for faulty instructions leading to crashes... ");

            std::vector<uintptr_t> faultyAddresses = Memory::Scan::Find(NullWrites, uintptr_t(GetModuleHandle(nullptr)));

            uintptr_t moduleBase = uintptr_t(GetModuleHandle(nullptr));

            uint32_t failedCount = 0;
            for (uintptr_t faultyAddress : faultyAddresses)
            {
                Assembly::Function function(faultyAddress);
                std::optional<uintptr_t> epilogueAddress = function.FindEpilogue();

                if (!epilogueAddress)
                {
                    ++failedCount;
                    console->error("Invalid memory write found at 0x{:x} (HorizonZeroDawn.exe+0x{:x}) but function epilogue was not detected.",
                        faultyAddress, faultyAddress - moduleBase);
                }
                else
                {
                    // Compute jump displacement for absolute jmp
                    // TODO: add more flavors of jmp, i'm just lazy
                    Assembly::Instruction faultyInstruction(faultyAddress);
                    uint32_t distance = uint32_t(epilogueAddress.value() - faultyAddress - 5);
                    if (distance < std::numeric_limits<uint32_t>::max())
                    {
                        uint8_t patchData[] = {
                            0xE9, 0x00, 0x00, 0x00, 0x00,           // JMP rel32
                            0x90, 0x90, 0x90, 0x90, 0x90, 0x90      // A bunch of NOPs for the patch to look nice
                        };

                        memcpy(&patchData[1], &distance, sizeof(uint32_t));
                        Memory::WriteMemory(faultyAddress, patchData, false);
                        Memory::FlushInstructionCache(faultyAddress, sizeof(patchData), false);

                        console->info("Invalid memory write patched to an early exit at 0x{:x} (HorizonZeroDawn.exe+0x{:x}).",
                            faultyAddress, faultyAddress - moduleBase);
                    }
                    else
                    {
                        ++failedCount;
                        console->error("Invalid memory write at 0x{:x} (HorizonZeroDawn.exe+0x{:x}) is too far away "
                            "from end of function ({} bytes) for a patch.", faultyAddress, std::abs(int32_t(distance)));
                    }
                }
            }

            SetUnhandledExceptionFilter(&UnhandledExceptionFilter);
            AddVectoredExceptionHandler(1 /* Call first */, &UnhandledExceptionFilter);
            console->info("Exception handler installed as a fallback for {} memory locations unable to be patched.", failedCount);
        }
    }
}