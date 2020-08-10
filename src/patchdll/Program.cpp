#include <iostream>
#include <cstdio>
#include <Windows.h>

void LockInProcess(HMODULE module)
{
    char moduleName[1024];
    if (0 == GetModuleFileNameA(module, moduleName, sizeof(moduleName) / sizeof(char)))
        return;

    // Load ourselves to increment the reference count.
    LoadLibraryA(moduleName);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(reason);
    UNREFERENCED_PARAMETER(lpReserved);

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        {
            LockInProcess(hModule);
#if _DEBUG
            AllocConsole();

            HWND consoleHandle = GetConsoleWindow();
            EnableMenuItem(GetSystemMenu(consoleHandle, false), SC_CLOSE, MF_GRAYED);
            DrawMenuBar(consoleHandle);

            (void) freopen("CONOUT$", "w", stdout);
#endif
            break;
        }
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}
