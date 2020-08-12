#include <filesystem>
#include <memory>

#include "Logger.hpp"
#include "Patches/DebugLogPatch.hpp"

#include <Windows.h>

#define EXPORT_INTERFACE __declspec(dllexport)

#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace fs = std::filesystem;

// A loader needs to call Initialize.

extern "C"
{
	EXPORT_INTERFACE DWORD WINAPI Initialize(LPVOID)
	{
		// Compiler may complain but this is to be perfectly sure of the exported function name.
		// WINAPI callconv is __stdcall which means in x86 the function gets a different name even with extern "C".
		// Apparently in x86-64 this isn't necessary.
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)

		// Log everything to file
		auto fileSink = Logger::Instance().RegisterSink<spdlog::sinks::basic_file_sink_mt>("patchdll.log");
		fileSink->set_level(spdlog::level::trace);

#if _DEBUG
		// Only errors to console
		auto outSink = Logger::Instance().RegisterSink<spdlog::sinks::stdout_color_sink_mt>();
		outSink->set_level(spdlog::level::debug);
#else
		// no console sink in release (no terminal created)
		spdlog::sink_ptr outSink;
#endif

		auto logger = Logger::Instance().RegisterLogger("console", { fileSink, outSink });
		auto gameLogger = Logger::Instance().RegisterLogger("game", { fileSink, outSink });

		logger->info("Log file '{}' opened.", fs::absolute(fileSink->filename()).string());
		
		Patches::DebugLogs::ApplyPatches();
		return EXIT_SUCCESS;
	}
}
