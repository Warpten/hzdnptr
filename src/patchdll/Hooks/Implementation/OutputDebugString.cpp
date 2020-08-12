#include "OutputDebugString.hpp"
#include "../../Logger.hpp"

namespace Hooks::Implementation
{
    DetourHandler<decltype(&OutputDebugString), LPCSTR> fnOutputDebugString(&OutputDebugString,
        [](LPCSTR lpOutputString) -> void {

            Logger::Instance().GetLogger("console")->debug(lpOutputString);

            return fnOutputDebugString.CallOriginal(lpOutputString);
        }
    );
}
