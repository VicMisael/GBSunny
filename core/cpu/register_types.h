//
// Created by visael on 14/03/25.
//

#ifndef REGISTER_TYPES_H
#define REGISTER_TYPES_H
#include <cstdint>

namespace cpu
{

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#endif
    union f_reg {
        uint8_t f;

        struct {
            uint8_t : 4; //LSB;
            bool CARRY: 1;
            bool HALF_CARRY: 1;
            bool SUBTRACT: 1;
            bool ZERO: 1; //MSB
        };

        struct {
            uint8_t  : 4; //LSB;
            bool C: 1;
            bool H: 1;
            bool N : 1;
            bool Z : 1; //MSB
        } by_mnemonic;

        f_reg& operator=(const uint8_t& input ) {
            f=input;
            return *this;
        }

        constexpr void set_flags(bool zero, bool subtract, bool half_carry, bool carry) {
            f = static_cast<uint8_t>(
                (static_cast<uint8_t>(zero) << 7) |
                (static_cast<uint8_t>(subtract) << 6) |
                (static_cast<uint8_t>(half_carry) << 5) |
                (static_cast<uint8_t>(carry) << 4));
        }

        void zero_unused_nibble() {
            this->f &= 0xf0;
        };

    };
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
}
#endif //REGISTER_TYPES_H
