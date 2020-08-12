#include "DetourManager.hpp"

#include <functional>
#include <stdexcept>

#include <windows.h>

namespace Hooks {
    /* static */ Registry& Registry::Instance() {
        static Registry instance;
        return instance;
    }

    Registry::Registry() {
        MH_Initialize();
    }

    Registry::~Registry() {
        MH_Uninitialize();
    }

    bool Registry::IsRegionHooked(uintptr_t from, uintptr_t to) {
        for (auto&& kv : _detours) {
            if (kv.first >= from && kv.first <= to)
                return true;
        }

        return false;
    }

    bool Registry::Delete(uintptr_t offset) {
        auto itr = _detours.find(offset);
        if (itr == _detours.end())
            return false;

        itr->second.RemoveHook();
        _detours.erase(itr);
        return true;
    }

    MH_STATUS Detour::CreateHook() {
        return MH_CreateHook(LPVOID(_offset), _installed, &_original);
    }

    void Detour::RemoveHook() {
        if (!_enabled)
            return;

        DisableHook();
        MH_RemoveHook(LPVOID(_offset));
    }

    MH_STATUS Detour::EnableHook() {
        if (_enabled)
            return MH_OK;

        _enabled = true;
        return MH_EnableHook(LPVOID(_offset));
    }

    void Detour::DisableHook() {
        if (_enabled)
            MH_DisableHook(LPVOID(_offset));

        _enabled = false;
    }

    bool Detour::IsEnabled() const {
        return _enabled;
    }
}
