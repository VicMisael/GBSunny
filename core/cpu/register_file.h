//
// Created by Misael on 08/03/2025.
//

#ifndef REGISTER_FILE_H
#define REGISTER_FILE_H
#include <bit>
#include <cstdint>
#include <iomanip>
#include <ostream>

#include "register_types.h"


namespace cpu {
    //Assumes LittleEndian
    static_assert(std::endian::native == std::endian::little,
        "register_file assumes little-endian layout");
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#endif
    class register_file {
    public:
        register_file();

        void reset();


        // uint8_t a, b, c, d, e, g, h, l;
        uint16_t pc;
        uint16_t sp;

        union {
            uint16_t af{};
            struct {
                f_reg f;
                uint8_t a;
            };
        };

        union {
            uint16_t bc{};
            struct {
                uint8_t c;
                uint8_t b;
            };
        };

        union {
            uint16_t de{};
            struct {
                uint8_t e;
                uint8_t d;
            };
        };

        union {
            uint16_t hl{};
            struct {
                uint8_t l;
                uint8_t h;
            };
        };


    public:
        void print_registers(std::ostream& output) const {
            // Set up formatting for hexadecimal output
            output << std::hex << std::uppercase << std::setfill('0');

            // Print 16-bit registers and their 8-bit components
            output << "AF: " << std::setw(4) << af
                << " (A: " << std::setw(2) << (int)a
                << " F: " << std::setw(2) << (int)f.f << ")" << std::endl;

            output << "BC: " << std::setw(4) << bc
                << " (B: " << std::setw(2) << (int)b
                << " C: " << std::setw(2) << (int)c << ")" << std::endl;

            output << "DE: " << std::setw(4) << de
                << " (D: " << std::setw(2) << (int)d
                << " E: " << std::setw(2) << (int)e << ")" << std::endl;

            output << "HL: " << std::setw(4) << hl
                << " (H: " << std::setw(2) << (int)h
                << " L: " << std::setw(2) << (int)l << ")" << std::endl;

            // Print Stack Pointer and Program Counter
            output << "SP: " << std::setw(4) << sp << std::endl;
            output << "PC: " << std::setw(4) << pc << std::endl;

            // Print the state of the flags
            const uint8_t flags = f.f;
            output << "Flags (ZNHC): "
                << ((flags & 0x80) ? '1' : '0') // Zero Flag
                << ((flags & 0x40) ? '1' : '0') // Subtract Flag
                << ((flags & 0x20) ? '1' : '0') // Half Carry Flag
                << ((flags & 0x10) ? '1' : '0') // Carry Flag
                << std::endl;

                // Reset formatting to default
            //output << std::dec << std::nouppercase << std::setfill(' ') << "--------------------" << std::endl;
        }

        // Exam
    };
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
}


#endif //REGISTER_FILE_H
