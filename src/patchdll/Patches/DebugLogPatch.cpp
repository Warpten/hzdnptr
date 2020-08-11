#include "DebugLogPatch.hpp"
#include "../Assembly/Function.hpp"
#include "../Assembly/Instruction.hpp"

#include "../Scan.hpp"
#include "../MemoryHelpers.hpp"

#include <fmt/format.h>
#include <iostream>

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
                    std::cout << fmt::format("Intercepted invalid memory write to 0x{:X}", exceptionInfo->ExceptionRecord->ExceptionInformation[1]) << std::endl;

                    // Silence the error
                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }

            // Keep searching for a handler
            return EXCEPTION_CONTINUE_SEARCH;
        }

        void ApplyPatches()
        {
            std::cout << "[+] Scanning executable code for faulty instructions leading to crashes... " << std::endl;

            std::vector<uintptr_t> faultyAddresses = Memory::Scan::Find(NullWrites, uintptr_t(GetModuleHandle(nullptr)));

            ZydisDecoder decoder;
            ZyanStatus result = ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);

            uintptr_t moduleBase = uintptr_t(GetModuleHandle(nullptr));

            for (uintptr_t faultyAddress : faultyAddresses)
            {
                Assembly::Function function(faultyAddress, decoder);
                std::optional<uintptr_t> epilogueAddress = function.FindEpilogue();

                if (!epilogueAddress)
                {
                    std::cout
                        << fmt::format("[+] Invalid memory write found at 0x{:x} (HorizonZeroDawn.exe+0x{:x}) but function epilogue was not detected.",
                            faultyAddress, faultyAddress - moduleBase)
                        << std::endl;
                    continue;
                }
                else
                {
                    // Compute jump displacement for absolute jmp
                    // TODO: add more flavors of jmp, i'm just lazy
                    Assembly::Instruction faultyInstruction(faultyAddress, decoder);
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

                        std::cout
                            << fmt::format("[+] Invalid memory write patched to an early exit at 0x{:x} (HorizonZeroDawn.exe+0x{:x}).",
                                faultyAddress, faultyAddress - moduleBase)
                            << std::endl;
                    }
                    else
                    {
                        std::cout
                            << fmt::format("[+] Invalid memory write at 0x{:x} (HorizonZeroDawn.exe+0x{:x}) is too far away "
                                "from end of function ({} bytes) for a patch.", faultyAddress, std::abs(int32_t(distance)))
                            << std::endl;
                    }

                }
            }

            SetUnhandledExceptionFilter(&UnhandledExceptionFilter);
            AddVectoredExceptionHandler(1 /* Call first */, &UnhandledExceptionFilter);
            std::cout << "[+] Exception handler installed." << std::endl;
        }
    }
}