#include "OutputDebugString.hpp"
#include "../../Logger.hpp"

#include <string_view>

namespace Hooks::Implementation
{
    DetourHandler<decltype(&OutputDebugString), LPCSTR> fnOutputDebugString(&OutputDebugString,
        [](LPCSTR lpOutputString) -> void {

            auto logger = Logger::Instance().GetLogger("game");

            std::string_view outputStringView = lpOutputString;

            auto idx = outputStringView.find('\n');
            if (idx == std::string_view::npos)
                logger->warn(outputStringView);
            else
            {
                while (idx != std::string_view::npos && outputStringView.length() != 0)
                {
                    auto substring = outputStringView.substr(0, idx);

                    if (substring.length() > 0)
                        logger->warn(substring);
                    outputStringView = outputStringView.substr(idx + 1);
                    idx = outputStringView.find('\n');
                }
            }

            return fnOutputDebugString.CallOriginal(lpOutputString);
        }
    );
}
