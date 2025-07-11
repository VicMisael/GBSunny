//
// Created by visael on 14/03/25.
//

#ifndef REGISTER_TYPES_H
#define REGISTER_TYPES_H
#include <cstdint>

namespace cpu
{

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
        void zero_unused_nibble() {
            this->f &= 0xf0;
        };

        void reset_all_flags() {
            this->f &= 0x0f;
        }
    };
}
#endif //REGISTER_TYPES_H
