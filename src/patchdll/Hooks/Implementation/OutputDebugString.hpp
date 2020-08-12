#ifndef HOOKS_IMPLEMENTATION_OUTPUTDEBUGSTRING_HPP__
#define HOOKS_IMPLEMENTATION_OUTPUTDEBUGSTRING_HPP__

#include "../DetourManager.hpp"

#include <windows.h>

namespace Hooks::Implementation
{
    extern DetourHandler<decltype(&OutputDebugString), LPCSTR> fnOutputDebugString;
}

#endif // HOOKS_IMPLEMENTATION_OUTPUTDEBUGSTRING_HPP__
