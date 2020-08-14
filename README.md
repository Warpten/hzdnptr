

Horizon: Zero Dawn - Nullptr Edition
====
This repository contains the code for an injectable DLL used to patch potential crashes due to null pointer dereferences (amongst other funsies) in Guerilla's recent Horizon: Zero Dawn PC release.

[![Build status](https://ci.appveyor.com/api/projects/status/eathqpg9rxjba75f?svg=true)](https://ci.appveyor.com/project/Warpten/hzdnptr)

## Current stable

[1.0.6](https://ci.appveyor.com/project/Warpten/hzdnptr/builds/34654694/artifacts)

## Disclaimer

```
THE CODE IN THIS REPOSITORY IS NOT ENDORSED NOR VALIDATED BY SONY NOR GUERILLA.
I CANNOT BE MADE RESPONSIBLE BY ISSUES THAT MAY ARISE FROM USING THIS SOFTWARE.
USE AT YOUR OWN RISK.
```

## Usage

1. Open the game.
2. Open an elevated command prompt.
3. Navigate to the folder in which injecter.exe is located.
4. Run the following command

```
injecter.exe -p HorizonZeroDawn.exe -d patchdll.dll
```

If everything goes according to plan, you should see the following output:
```
[!] Allocating 500 bytes of data.  
[+] Finding kernel32!LoadLibrary.  
[+] Executing 'Initialize'.
[+] Initialization successful.
```

## Building

#### Cloning
```
git clone --recurse-submodules http://github.com/Warpten/hzdnptr.git
```

This project requires [CMake](https://cmake.org/).
When generating the solution, uncheck `ZYDIS_BUILD_EXAMPLES` as well as `ZYDIS_BUILD_TOOLS`.
If you're planning to use `spdlog`, enable `SPDLOG_FMT_EXTERNAL`.

#### Dependencies

|                                             | Injecter           | Patch DLL          |
|---------------------------------------------|--------------------|--------------------|
| Boost (>= 1.66)                             | :white_check_mark: |                    |
| minhook                                     |                    | :white_check_mark: |
| [fmt](https://github.com/fmtlib/fmt)        |                    | :white_check_mark: |
| [Zydis](https://github.com/zyantific/zydis) |                    | :white_check_mark: |
| [spdlog](https://github.com/gabime/spdlog)  |                    | :white_check_mark: |

A compiler with C++17 support is required.

:information_source: These dependencies are fulfilled via submodules.


## Patches applied

Did you see an issue that this project did not fix? Head over to [Issues](https://github.com/Warpten/hzdnptr/issues) and please provide a `.DMP` file of your error.

##### `mov     dword ptr ds:0, 0DEADCA7h`

We scan the game's code for the above instruction, and attempt to find the enclosing function's epilogue. When found, we patch the instruction, jumping instead to the function's epilogue. If finding the epilogue is impossible (in case of custom calling conventions or non-standard epilogues generated by the compiler), we also install an exception handler that intercepts invalid memory writes and continues execution. This approach is less robust but should help stabilize the game until Guerilla fixes it.

#### `OutputDebugString`

This one does not fix anything but it's for my own curiosity. `OutputDebugString` is detoured so that its arguments are logged by the DLL. This Windows API is used exclusively for logging strings to a debugger, if one is attached to the running process. If none is found, you can still see those messages with tools such as [DbgView](https://docs.microsoft.com/en-us/sysinternals/downloads/debugview). Showing them in the DLL's logs is purely for curiosity.

## Known issues

Some epilogues fail detection even though there is no apparent reason for that to be the case.

## Copyright

This is a point of contention but most of the code here is mine unless otherwise specified in the source files themselves. You're free to reuse **my** code as you please (credit would be nice!). Please refer to the original authors if you wish to use other bits of code.
