// compiler.hpp
#pragma once

#if defined(_MSC_VER)
    #define NO_INLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
    #define NO_INLINE __attribute__((noinline))
#else
    #define NO_INLINE
#endif