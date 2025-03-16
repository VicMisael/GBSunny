//
// Created by Misael on 15/03/2025.
//

#ifndef TYPES_H
#define TYPES_H
#include <cstdint>
#include <bitset>
namespace shared{
    union interrupt_register{
        uint8_t flag;
        //std::bitset<8> bitset;
        struct {
            bool VBlank : 1;
            bool LCD: 1;
            bool timer : 1;
            bool serial : 1;
            bool joypad : 1;
            uint8_t _padding : 3;
        };
    };
}
#endif //TYPES_H
