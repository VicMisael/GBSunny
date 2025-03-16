//
// Created by Misael on 08/03/2025.
//

#ifndef REGISTER_FILE_H
#define REGISTER_FILE_H
#include <cstdint>

#include "register_types.h"


namespace cpu {
    class register_file {
    public:
        register_file();

        void reset();


        // uint8_t a, b, c, d, e, g, h, l;
        uint16_t pc;
        uint16_t sp;

        union {
            uint16_t af;
            struct {
                f_reg f;
                uint8_t a;
            };
        };

        union {
            uint16_t bc;
            struct {
                uint8_t c;
                uint8_t b;
            };
        };

        union {
            uint16_t de;
            struct {
                uint8_t e;
                uint8_t d;
            };
        };

        union {
            uint16_t hl;
            struct {
                uint8_t l;
                uint8_t h;
            };
        };
    };
}


#endif //REGISTER_FILE_H
