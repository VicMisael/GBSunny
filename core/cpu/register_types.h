//
// Created by visael on 14/03/25.
//

#ifndef REGISTER_TYPES_H
#define REGISTER_TYPES_H
#include <cstdint>
#include <type_traits>

namespace cpu
{

    class f_reg {
    public:
        enum class flag : uint8_t {
            carry = 0x10,
            half_carry = 0x20,
            subtract = 0x40,
            zero = 0x80,
        };

        constexpr f_reg() noexcept = default;

        constexpr explicit f_reg(const uint8_t value) noexcept
            : value_(value & valid_flag_mask) {
        }

        constexpr f_reg& operator=(const uint8_t value) noexcept {
            value_ = value & valid_flag_mask;
            return *this;
        }

        [[nodiscard]] constexpr explicit operator uint8_t() const noexcept {
            return value_;
        }

        [[nodiscard]] constexpr bool test(const flag selected) const noexcept {
            return (value_ & mask(selected)) != 0;
        }

        constexpr void set(const flag selected, const bool enabled) noexcept {
            const uint8_t selected_mask = mask(selected);
            value_ = enabled
                ? static_cast<uint8_t>(value_ | selected_mask)
                : static_cast<uint8_t>(value_ & static_cast<uint8_t>(~selected_mask));
        }

        [[nodiscard]] constexpr bool carry() const noexcept {
            return test(flag::carry);
        }

        constexpr void carry(const bool enabled) noexcept {
            set(flag::carry, enabled);
        }

        [[nodiscard]] constexpr bool half_carry() const noexcept {
            return test(flag::half_carry);
        }

        constexpr void half_carry(const bool enabled) noexcept {
            set(flag::half_carry, enabled);
        }

        [[nodiscard]] constexpr bool subtract() const noexcept {
            return test(flag::subtract);
        }

        constexpr void subtract(const bool enabled) noexcept {
            set(flag::subtract, enabled);
        }

        [[nodiscard]] constexpr bool zero() const noexcept {
            return test(flag::zero);
        }

        constexpr void zero(const bool enabled) noexcept {
            set(flag::zero, enabled);
        }

        [[nodiscard]] constexpr uint8_t upper_nibble() const noexcept {
            return value_ >> 4;
        }

        constexpr void reset_all_flags() noexcept {
            value_ = 0;
        }

    private:
        static constexpr uint8_t valid_flag_mask = 0xF0;

        [[nodiscard]] static constexpr uint8_t mask(const flag selected) noexcept {
            return static_cast<uint8_t>(selected);
        }

        uint8_t value_{};
    };

    template<typename LowRegister = uint8_t>
        requires std::is_assignable_v<LowRegister&, uint8_t>
    class register_pair_ref {
    public:
        constexpr register_pair_ref(uint8_t& upper, LowRegister& lower) noexcept
            : upper_(upper), lower_(lower) {
        }

        constexpr register_pair_ref& operator=(const register_pair_ref& other) noexcept {
            return *this = static_cast<uint16_t>(other);
        }

        template<typename OtherLowRegister>
        constexpr register_pair_ref& operator=(
            const register_pair_ref<OtherLowRegister>& other
        ) noexcept {
            return *this = static_cast<uint16_t>(other);
        }

        constexpr register_pair_ref& operator=(const uint16_t value) noexcept {
            upper_ = static_cast<uint8_t>(value >> 8);
            lower_ = static_cast<uint8_t>(value);
            return *this;
        }

        [[nodiscard]] constexpr operator uint16_t() const noexcept {
            return combine(upper_, static_cast<uint8_t>(lower_));
        }

    private:
        [[nodiscard]] static constexpr uint16_t combine(
            const uint8_t upper,
            const uint8_t lower
        ) noexcept {
            return static_cast<uint16_t>((uint16_t{upper} << 8) | lower);
        }

        uint8_t& upper_;
        LowRegister& lower_;
    };
}
#endif //REGISTER_TYPES_H
