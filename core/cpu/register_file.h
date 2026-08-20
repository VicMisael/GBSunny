//
// Created by Misael on 08/03/2025.
//

#ifndef REGISTER_FILE_H
#define REGISTER_FILE_H
#include <cstdint>
#include <iomanip>
#include <ostream>

#include "register_types.h"


namespace cpu {
    class register_file {
    public:
        register_file();

        void reset();


        uint16_t pc{};
        uint16_t sp{};

        uint8_t a{};
        f_reg f{};
        uint8_t b{};
        uint8_t c{};
        uint8_t d{};
        uint8_t e{};
        uint8_t h{};
        uint8_t l{};

        [[nodiscard]] constexpr register_pair_ref<f_reg> af() noexcept {
            return {a, f};
        }

        [[nodiscard]] constexpr register_pair_ref<> bc() noexcept {
            return {b, c};
        }

        [[nodiscard]] constexpr register_pair_ref<> de() noexcept {
            return {d, e};
        }

        [[nodiscard]] constexpr register_pair_ref<> hl() noexcept {
            return {h, l};
        }

        [[nodiscard]] constexpr uint16_t af() const noexcept {
            return combine(a, static_cast<uint8_t>(f));
        }

        [[nodiscard]] constexpr uint16_t bc() const noexcept {
            return combine(b, c);
        }

        [[nodiscard]] constexpr uint16_t de() const noexcept {
            return combine(d, e);
        }

        [[nodiscard]] constexpr uint16_t hl() const noexcept {
            return combine(h, l);
        }

        void print_registers(std::ostream& output) const {
            // Set up formatting for hexadecimal output
            output << std::hex << std::uppercase << std::setfill('0');

            // Print 16-bit registers and their 8-bit components
            output << "AF: " << std::setw(4) << af()
                << " (A: " << std::setw(2) << (int)a
                << " F: " << std::setw(2) << static_cast<int>(static_cast<uint8_t>(f)) << ")" << std::endl;

            output << "BC: " << std::setw(4) << bc()
                << " (B: " << std::setw(2) << (int)b
                << " C: " << std::setw(2) << (int)c << ")" << std::endl;

            output << "DE: " << std::setw(4) << de()
                << " (D: " << std::setw(2) << (int)d
                << " E: " << std::setw(2) << (int)e << ")" << std::endl;

            output << "HL: " << std::setw(4) << hl()
                << " (H: " << std::setw(2) << (int)h
                << " L: " << std::setw(2) << (int)l << ")" << std::endl;

            // Print Stack Pointer and Program Counter
            output << "SP: " << std::setw(4) << sp << std::endl;
            output << "PC: " << std::setw(4) << pc << std::endl;

            // Print the state of the flags
            const uint8_t flags = static_cast<uint8_t>(f);
            output << "Flags (ZNHC): "
                << ((flags & 0x80) ? '1' : '0') // Zero Flag
                << ((flags & 0x40) ? '1' : '0') // Subtract Flag
                << ((flags & 0x20) ? '1' : '0') // Half Carry Flag
                << ((flags & 0x10) ? '1' : '0') // Carry Flag
                << std::endl;

                // Reset formatting to default
            //output << std::dec << std::nouppercase << std::setfill(' ') << "--------------------" << std::endl;
        }

    private:
        [[nodiscard]] static constexpr uint16_t combine(
            const uint8_t upper,
            const uint8_t lower
        ) noexcept {
            return static_cast<uint16_t>((uint16_t{upper} << 8) | lower);
        }
    };
}


#endif //REGISTER_FILE_H
