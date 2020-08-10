#include <memory>

#include "Patches/DebugLogPatch.hpp"

#include <Windows.h>

#define EXPORT_INTERFACE __declspec(dllexport)

// A loader needs to call Initialize.

extern "C"
{
	EXPORT_INTERFACE DWORD WINAPI Initialize(LPVOID)
	{
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)

		Patches::DebugLogs::ApplyPatches();

		return EXIT_SUCCESS;
	}
}
