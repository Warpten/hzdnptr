#include "Library.hpp"
#include "Process.hpp"

#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include <Windows.h>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

struct InitializationChainElement
{
    InitializationChainElement(std::shared_ptr<Process> process, const std::string_view functionName)
        : _process(process), _fptr(0), _functionName(functionName), _next(nullptr)
    {

    }

    virtual ~InitializationChainElement()
    {
        if (_next)
            delete _next;
    }

    bool TryRead(const char* functionName, uintptr_t fptr)
    {
        if (_functionName == functionName)
        {
            _fptr = fptr;
            return true;
        }

        if (_next)
            return _next->TryRead(functionName, fptr);

        return false;
    }

    bool Validate()
    {
        return _fptr != 0 && (!_next || _next->Validate());
    }

    template <typename T, typename... Args>
    InitializationChainElement& ContinueWith(Args&&... args) {
        _next = new T(std::forward<Args>(args)...);
        return *_next;
    }

    DWORD Call() const
    {
        if (!_process)
            return EXIT_FAILURE;

        DWORD result = CreateRemoteThread();
        if (result == EXIT_FAILURE)
            return EXIT_FAILURE;

        if (_next)
            return _next->Call();

        return result;
    }

protected:
    virtual DWORD CreateRemoteThread() const
    {
        std::cout << "[+] Executing '" << _functionName << "'." << std::endl;

        return _process->CreateRemoteThread<DWORD>(_fptr, 0, false);
    }

    std::shared_ptr<Process> _process;
    std::string _functionName;
    uintptr_t _fptr;
    InitializationChainElement* _next;
};

template <typename T>
struct ArgInitializationChainElement : public InitializationChainElement
{
    ArgInitializationChainElement(std::shared_ptr<Process> process, const std::string_view functionName, uintptr_t arg, T& value)
        : InitializationChainElement(process, functionName), _arg(arg), _value(value)
    {

    }

    template <typename U>
    InitializationChainElement& ContinueWith(const std::string_view functionName, uintptr_t arg, U& value)
    {
        return ContinueWith(ArgInitializationChainElement<U>{ functionName, arg, value});
    }

protected:
    virtual DWORD CreateRemoteThread() const override
    {
        std::cout << "[+] Writing '" << _value << "' to target memory." << std::endl;

        if constexpr (std::is_same_v<T, std::string>)
        {
            if (!_process->WriteCString(_arg, _value, false))
                return EXIT_FAILURE;
        }
        else
        {
            if (!_process->Write(_arg, _value, sizeof(T), false))
                return EXIT_FAILURE;
        }

        std::cout << "[+] Executing '" << _functionName << "'." << std::endl;
        return _process->CreateRemoteThread<DWORD>(_fptr, _arg, false);
    }

private:
    uintptr_t _arg;
    T& _value;
};

void inject(po::variables_map const& vm);

int main(int argc, char** argv)
{
    po::options_description options("Injecter options");
    po::variables_map vm;
    try {
        options.add_options()
            ("process,p", po::value<std::string>(), "Process name to attach to.")
            ("dll,d", po::value<std::string>()->required(), "Path to the DLL to inject.")
            ("help,h", "Show this help message.");

        po::store(po::parse_command_line(argc, argv, options), vm);
        po::notify(vm);

        if (vm.empty() || vm.count("help") > 0)
        {
            std::cout << options << std::endl;
            return EXIT_FAILURE;
        }
    }
    catch (po::error const&) {
        std::cout << options << std::endl;
        return EXIT_FAILURE;
    }

    inject(vm);
    std::cout << std::endl;
    return EXIT_SUCCESS;
}

void inject(po::variables_map const& vm)
{
    using namespace std::literals;

    std::string dllName = vm["dll"].as<std::string>();
    std::string absoluteDllPath = std::filesystem::absolute(vm["dll"].as<std::string>()).string();

    if (auto remoteProcess = ProcessTools::Open(vm["process"].as<std::string>(), 0, false))
    {
        // Prepare a chunk of memory we will reuse for the various steps of DLL injection and initialization.
        size_t allocationSize = 500;
        allocationSize = std::max(allocationSize, absoluteDllPath.length() + 1);
        allocationSize = std::max(allocationSize, "kernel32.dll"sv.size() + 1);
        allocationSize = std::max(allocationSize, "GetProcAddress"sv.size() + 1);
        allocationSize = std::max(allocationSize, "LoadLibraryA"sv.size() + 1);

        std::cout << "[!] Allocating " << allocationSize << " bytes of data." << std::endl;
        std::uintptr_t allocatedMemory = remoteProcess->AllocateMemory(allocationSize);
        if (allocatedMemory == 0u)
        {
            std::cerr << "[!] Failed to allocate " << allocationSize << " bytes of data." << std::endl;
            return;
        }

        // RAII abuse to release the allocated memory block in exit
        std::shared_ptr<void> allocatedMemoryRAII(nullptr, [&](void*) { remoteProcess->FreeMemory(allocatedMemory, allocationSize); });

        // We can abuse the simple fact that kernel32.dll is always loaded at the same RAM address to call it in the remote process.
        std::cout << "[+] Finding kernel32!LoadLibrary." << std::endl;
        Library kernelModule("kernel32.dll");
        std::uintptr_t loadLibraryA = kernelModule.GetProcOffset("LoadLibraryA", false);

        // We now have a function pointer; all we need is to spawn a remote thread that calls that function with given arguments.
        if (!remoteProcess->WriteCString(allocatedMemory, absoluteDllPath, false))
        {
            std::cerr << "[!] Injection failed." << std::endl;
            return;
        }

        auto fn = reinterpret_cast<uintptr_t>(&LoadLibrary);

        // And now we call LoadLibrary remotely.
        // Masking is needed in 64-bits processe because CreateRemoteThread returns a pointer, but HMODULE fits in an integer
        // so we get 0xCCCCCCCC________ where ________ is the actual value.
        uintptr_t base = remoteProcess->CreateRemoteThread<std::uintptr_t>(loadLibraryA, allocatedMemory, false);

        uintptr_t remoteModuleBase = 0;
        auto remoteModules = remoteProcess->EnumerateModules();
        for (Process::Module& remoteModule : remoteModules)
        {
            if (remoteModule.Name == dllName)
                remoteModuleBase = uintptr_t(remoteModule.Base);
        }

        if (remoteModuleBase == 0u)
        {
            std::uintptr_t getLastError = kernelModule.GetProcOffset("GetLastError", false);
            std::uint32_t errorCode = remoteProcess->CreateRemoteThread<std::uint32_t>(getLastError, 0, false);

            std::cerr << "[+] Remote execution of kernel32!LoadLibraryA failed with error code 0x" << std::hex << errorCode << "." << std::endl;

            return;
        }

        // The call to LoadLibrary returned the module's base address.
        // Now, all we need to do is walk the export table, and call the only function we will find.
        IMAGE_DOS_HEADER const& dosHeader = remoteProcess->Read<IMAGE_DOS_HEADER>(remoteModuleBase, false);
        IMAGE_NT_HEADERS const& ntHeader = remoteProcess->Read<IMAGE_NT_HEADERS>(remoteModuleBase + dosHeader.e_lfanew, false);

        IMAGE_EXPORT_DIRECTORY const& exportDirectory = remoteProcess->Read<IMAGE_EXPORT_DIRECTORY>(remoteModuleBase + ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress, false);

        if (exportDirectory.NumberOfFunctions == 0 || exportDirectory.NumberOfNames == 0)
        {
            std::cout << "[+] No export found in DLL." << std::endl;
            return;
        }

        std::vector<DWORD> exportNames = remoteProcess->ReadArray<DWORD>(remoteModuleBase + exportDirectory.AddressOfNames, exportDirectory.NumberOfNames, false);
        std::vector<WORD> exportOrdinals = remoteProcess->ReadArray<WORD>(remoteModuleBase + exportDirectory.AddressOfNameOrdinals, exportDirectory.NumberOfNames, false);
        std::vector<DWORD> exportPtrs = remoteProcess->ReadArray<DWORD>(remoteModuleBase + exportDirectory.AddressOfFunctions, exportDirectory.NumberOfFunctions, false);

        // Setup a call chain for DLL initialization.
        InitializationChainElement callChain(remoteProcess, "Initialize");

        bool initializationSuccess = false;
        for (uint32_t i = 0; i < exportDirectory.NumberOfNames; ++i)
        {
            std::string functionName = remoteProcess->ReadString(remoteModuleBase + exportNames[i], false);
            std::uintptr_t functionAddress = exportPtrs[exportOrdinals[i]];

            callChain.TryRead(functionName.data(), functionAddress + remoteModuleBase);
        }

        if (!callChain.Validate() || callChain.Call() == EXIT_FAILURE)
        {
            std::cerr << "[!] Initialization failed." << std::endl;
            return;
        }

        std::cout << "[+] Initialization successful." << std::endl;
        return;
    }

    std::cerr << "[!] Process not found." << std::endl;
}