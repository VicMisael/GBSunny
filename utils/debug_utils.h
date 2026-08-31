#pragma once

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <csignal>
#endif

class DebugUtils final {
public:
    DebugUtils() = delete;

    static void breakpoint() noexcept {
#if defined(_MSC_VER)
        __debugbreak();
#elif defined(__unix__) || defined(__APPLE__)
        std::raise(SIGTRAP);
#elif defined(__clang__) || defined(__GNUC__)
        __builtin_trap();
#endif
    }
};
