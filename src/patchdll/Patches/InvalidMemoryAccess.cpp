#include "InvalidMemoryAccess.hpp"
#include "../Logger.hpp"

#include <windows.h>

namespace Patches::InvalidMemoryAccess
{
    LONG WINAPI UnhandledExceptionFilter(_EXCEPTION_POINTERS* exceptionInfo)
    {
        if (exceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
        {
            auto console = Logger::Instance().GetLogger("console");

            uintptr_t moduleBase = uintptr_t(GetModuleHandle(nullptr));

            // Invalid memory write
            std::string operation;
            uintptr_t address = uintptr_t(exceptionInfo->ExceptionRecord->ExceptionInformation[1]);
            if (exceptionInfo->ExceptionRecord->ExceptionInformation[0] == 1)
                operation = "write";
            else if (exceptionInfo->ExceptionRecord->ExceptionInformation[0] == 0)
                operation = "read";

            console->error("HorizonZeroDawn.exe+0x{:x}: Invalid memory {} from/to address 0x{:x}.",
                exceptionInfo->ContextRecord->Rip - moduleBase, operation, address);

            // Silence the error
            return EXCEPTION_CONTINUE_EXECUTION;
        }

        // Keep searching for a handler
        return EXCEPTION_CONTINUE_SEARCH;
    }

    void Apply()
    {
        SetUnhandledExceptionFilter(&UnhandledExceptionFilter);
        AddVectoredExceptionHandler(1 /* Call first */, &UnhandledExceptionFilter);

        auto console = Logger::Instance().GetLogger("console");
        console->info("SEH and VEH installed.");
    }
}
