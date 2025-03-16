//
// Created by Misael on 08/03/2025.
//

#ifndef MMU_H
#define MMU_H
#include <cstdint>
#include <memory>

#include "cartridge/cartridge.h"
#include "ppu/ppu.h"
#include "timer/timer.h"
#include "shared/interrupt.h"

class spu;

namespace mmu {
    class MMU {
        uint8_t internal_RAM[4096] = {};
        uint8_t internal_RAM2[4096] = {}; // CGB
        uint8_t HRAM[0x80] = {};
        PPU _ppu;
        uint8_t temp = 0;

        uint8_t bootRomControl = 0x0;

        std::shared_ptr<timer> _timer;
        std::shared_ptr<Cartridge> cartridge;
        std::shared_ptr<spu> _spu;
        std::shared_ptr<shared::interrupt> interrupt; //Shared space for interrupts


        uint8_t read_interrupt_enable() const;
        uint8_t read_interrupt_flag() const;
        void set_interrupt_flag(uint8_t);
        void set_interrupt_enable(uint8_t);


        uint8_t io_read(uint16_t addr) const;
        void io_write(uint16_t addr, uint8_t data);

        [[nodiscard]] bool boot_rom_enabled() const;

    public:
        MMU(const std::shared_ptr<Cartridge> &cartridge, const PPU &ppu): _ppu(ppu), cartridge(cartridge) {
        };

        void reset();

        [[nodiscard]] uint8_t read(uint16_t addr) const ;

        uint8_t &read_as_ref(uint16_t addr);

        void write(uint16_t addr, const uint8_t &data);


    };
};


#endif //MMU_H
