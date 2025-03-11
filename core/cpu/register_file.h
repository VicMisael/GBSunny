//
// Created by Misael on 08/03/2025.
//

#ifndef REGISTER_FILE_H
#define REGISTER_FILE_H
#include <cstdint>
#include <bitset>

namespace cpu {
    class register_file {
    public:
        register_file();

        void reset();
        union f_reg {
            uint8_t f;
            
            struct {
                uint8_t _ignored: 4; //LSB;
                bool CARRY: 1;
                bool HALF_CARRY: 1;
                bool SUBTRACT: 1;
                bool ZERO: 1; //MSB
            };

            struct {
                uint8_t _ignored : 4; //LSB;
                bool C: 1;
                bool H: 1;
                bool N : 1;
                bool Z : 1; //MSB
            } by_mnemonic;

                f_reg& operator=(const uint8_t& input ) {
                    f=input;
                    return *this;
                }
                void inline zeroAll() {
                    this->f = 0;
                };
        };




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
