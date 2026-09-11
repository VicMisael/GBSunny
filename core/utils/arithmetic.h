#pragma once

#include <cstdint>
#include <type_traits>

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#endif

namespace utils
{
    template<typename T>
    struct arithmetic_result {
        T value;
        bool carry;
    };

    template<typename T>
    [[nodiscard]] constexpr arithmetic_result<T> add_carry(
        T lhs, T rhs, bool carry_in = false) noexcept {
        static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<T, bool>,
            "add_carry requires an unsigned integer type");

        if (std::is_constant_evaluated()) {
            const T intermediate = static_cast<T>(lhs + rhs);
            const bool first_carry = intermediate < lhs;
            const T result = static_cast<T>(intermediate + static_cast<T>(carry_in));
            return {result, first_carry || result < intermediate};
        }

#if defined(__GNUC__) || defined(__clang__)
        T intermediate;
        T result;
        const bool first_carry = __builtin_add_overflow(lhs, rhs, &intermediate);
        const bool second_carry = __builtin_add_overflow(intermediate, static_cast<T>(carry_in), &result);
        return {result, first_carry || second_carry};
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
        T result;
        unsigned char carry;
        if constexpr (sizeof(T) == sizeof(unsigned char)) {
            carry = _addcarry_u8(carry_in, lhs, rhs, reinterpret_cast<unsigned char*>(&result));
        } else if constexpr (sizeof(T) == sizeof(unsigned short)) {
            carry = _addcarry_u16(carry_in, lhs, rhs, reinterpret_cast<unsigned short*>(&result));
        } else if constexpr (sizeof(T) == sizeof(unsigned int)) {
            carry = _addcarry_u32(carry_in, lhs, rhs, reinterpret_cast<unsigned int*>(&result));
#if defined(_M_X64)
        } else if constexpr (sizeof(T) == sizeof(unsigned __int64)) {
            carry = _addcarry_u64(carry_in, lhs, rhs, reinterpret_cast<unsigned __int64*>(&result));
#endif
        } else {
            const T intermediate = static_cast<T>(lhs + rhs);
            const bool first_carry = intermediate < lhs;
            result = static_cast<T>(intermediate + static_cast<T>(carry_in));
            carry = first_carry || result < intermediate;
        }
        return {result, carry != 0};
#else
        const T intermediate = static_cast<T>(lhs + rhs);
        const bool first_carry = intermediate < lhs;
        const T result = static_cast<T>(intermediate + static_cast<T>(carry_in));
        return {result, first_carry || result < intermediate};
#endif
    }

    template<typename T>
    [[nodiscard]] constexpr arithmetic_result<T> subtract_carry(
        T lhs, T rhs, bool borrow_in = false) noexcept {
        static_assert(std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<T, bool>,
            "subtract_carry requires an unsigned integer type");

        if (std::is_constant_evaluated()) {
            const T intermediate = static_cast<T>(lhs - rhs);
            const bool first_borrow = lhs < rhs;
            const T result = static_cast<T>(intermediate - static_cast<T>(borrow_in));
            return {result, first_borrow || intermediate < static_cast<T>(borrow_in)};
        }

#if defined(__GNUC__) || defined(__clang__)
        T intermediate;
        T result;
        const bool first_borrow = __builtin_sub_overflow(lhs, rhs, &intermediate);
        const bool second_borrow = __builtin_sub_overflow(intermediate, static_cast<T>(borrow_in), &result);
        return {result, first_borrow || second_borrow};
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
        T result;
        unsigned char borrow;
        if constexpr (sizeof(T) == sizeof(unsigned char)) {
            borrow = _subborrow_u8(borrow_in, lhs, rhs, reinterpret_cast<unsigned char*>(&result));
        } else if constexpr (sizeof(T) == sizeof(unsigned short)) {
            borrow = _subborrow_u16(borrow_in, lhs, rhs, reinterpret_cast<unsigned short*>(&result));
        } else if constexpr (sizeof(T) == sizeof(unsigned int)) {
            borrow = _subborrow_u32(borrow_in, lhs, rhs, reinterpret_cast<unsigned int*>(&result));
#if defined(_M_X64)
        } else if constexpr (sizeof(T) == sizeof(unsigned __int64)) {
            borrow = _subborrow_u64(borrow_in, lhs, rhs, reinterpret_cast<unsigned __int64*>(&result));
#endif
        } else {
            const T intermediate = static_cast<T>(lhs - rhs);
            const bool first_borrow = lhs < rhs;
            result = static_cast<T>(intermediate - static_cast<T>(borrow_in));
            borrow = first_borrow || intermediate < static_cast<T>(borrow_in);
        }
        return {result, borrow != 0};
#else
        const T intermediate = static_cast<T>(lhs - rhs);
        const bool first_borrow = lhs < rhs;
        const T result = static_cast<T>(intermediate - static_cast<T>(borrow_in));
        return {result, first_borrow || intermediate < static_cast<T>(borrow_in)};
#endif
    }
}
