#ifndef HOOKS_DETOUR_MGR_HPP__
#define HOOKS_DETOUR_MGR_HPP__

#include <MinHook.h>

#include <array>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <stdexcept>
#include <string>

namespace Hooks
{
    struct Registry;

    struct Detour {
        template <typename T>
        constexpr Detour(uintptr_t offset, T trampoline)
            : _offset(offset), _installed(trampoline), _enabled(false), _original(nullptr)
        {
            memcpy(_originalBytes.data(), reinterpret_cast<void*>(offset), _originalBytes.size());
        }

        ~Detour() {
            RemoveHook();
        }

        Detour(Detour&& other) noexcept {
            _offset = other._offset;
            _installed = other._installed;
            _enabled = other._enabled;
            _original = other._original;

            _originalBytes = std::move(other._originalBytes);

            other._offset = 0;
            other._installed = nullptr;
            other._original = nullptr;
            other._enabled = false;
        }

    private:

        friend struct Registry;

        MH_STATUS CreateHook();
        void RemoveHook();
        MH_STATUS EnableHook();
        void DisableHook();

        std::array<uint16_t, 32> _originalBytes;

    public:
        bool IsEnabled() const;

        template <typename HandlerType, typename ReturnType, typename... Args>
        ReturnType CallOriginal(Args&&... args) {
            return reinterpret_cast<HandlerType>(_original)(std::forward<Args>(args)...);
        }

    private:
        DWORD_PTR _offset;
        LPVOID _original;
        LPVOID _installed;
        bool _enabled;
    };


    struct Registry final {

        static Registry& Instance();

        template <typename T>
        bool Register(uintptr_t offset, T trampoline, T* original = nullptr) {
            Detour detour(offset, trampoline);

            MH_STATUS status = detour.CreateHook();
            if (status != MH_OK) {
                throw std::runtime_error(MH_StatusToString(status));
            }
            else {
                if (original != nullptr)
                    *original = reinterpret_cast<T>(detour._original);

                if (status == MH_OK)
                    status = detour.EnableHook();

                _detours.emplace(offset, std::move(detour));
            }

            return status == MH_OK;
        }

        bool Delete(uintptr_t offset);

        bool IsRegionHooked(uintptr_t from, uintptr_t to);

        ~Registry();

    private:
        Registry();

        std::unordered_map<uintptr_t, Detour> _detours;
    };

    template <typename Fn, typename... Args>
    struct DetourHandler final {
        using function_type = Fn;

        DetourHandler(function_type source, function_type trampoline) : DetourHandler(trampoline) {
            Register(uintptr_t(reinterpret_cast<uint8_t*>(source)));
        }

        DetourHandler(uintptr_t source, function_type trampoline) : DetourHandler(trampoline) {
            Register(source);
        }

        DetourHandler() = delete;
        explicit DetourHandler(function_type trampoline) : _originalFunction(), _trampoline(trampoline), _removalOffset(0) { }

        bool Register(uintptr_t offset) {
            bool success = Registry::Instance().Register(offset, _trampoline, &_originalFunction);
            if (success)
                _removalOffset = offset;
            return success;
        }

        bool IsRegistered() const {
            return _removalOffset != 0;
        }

        bool Remove() {
            if (_removalOffset == 0)
                return true;

            bool result = Registry::Instance().Delete(_removalOffset);
            _removalOffset = 0;
            return result;
        }

        auto CallOriginal(Args... args) {
            return _originalFunction(std::forward<Args>(args)...);
        }

    private:
        uintptr_t _removalOffset;
        function_type _trampoline;
        function_type _originalFunction;
    };

}

#endif // DETOUR_MGR