//
// Created by Misael on 08/03/2025.
//

#ifndef MMU_H
#define MMU_H
#include <cstdint>
#include <memory>

#include "cartridge/cartridge.h"
#include "spu/spu.h"
#include "timer/gb_timer2.h"
#include "shared/interrupt.h"
#include "serial/gb_serial.h"
#include <ppu/ppu_base.h>



namespace mmu {
    class MMU {

        uint8_t internal_RAM[4096];
        uint8_t internal_RAM2[4096] ; // CGB
        uint8_t HRAM[128];



        uint8_t bootRomControl = 0;

        std::shared_ptr<PPU_Base> _ppu;
        std::shared_ptr<base_timer> _timer;
        std::shared_ptr<Cartridge> _cartridge;
        std::shared_ptr<spu> _spu;
        std::shared_ptr<serial::GBSerial> _serial;
        std::shared_ptr<shared::interrupt> interrupt; //Shared space for interrupts


        [[nodiscard]] uint8_t read_interrupt_enable() const;
        [[nodiscard]] uint8_t read_interrupt_flag() const;
        void set_interrupt_flag(uint8_t);
        void set_interrupt_enable(uint8_t);

        void oam_transfer(uint8_t params) const;


        [[nodiscard]] uint8_t io_read(uint16_t addr) const;
        void io_write(uint16_t addr, uint8_t data);

    public:
        MMU(const std::shared_ptr<Cartridge>& cart,
            std::shared_ptr<PPU_Base> ppu_ptr,
            const std::shared_ptr<base_timer>& timer_ptr,
            const std::shared_ptr<shared::interrupt>& interrupt_ptr,
            const std::shared_ptr<spu>& spu_ptr,
            std::shared_ptr<serial::GBSerial> serial_ptr
        ) : _ppu(std::move(ppu_ptr)), _cartridge(cart), _timer(timer_ptr),
            interrupt(interrupt_ptr),
            _spu(spu_ptr),
            _serial(std::move(serial_ptr)) {
            // Constructor body can be empty if all initialization is done in the list.
            std::fill(std::begin(internal_RAM), std::end(internal_RAM), 0);
            std::fill(std::begin(internal_RAM2), std::end(internal_RAM2), 0);
            std::fill(std::begin(HRAM), std::end(HRAM), 0);
        }
        [[nodiscard]] bool boot_rom_enabled() const;

        void reset();

        [[nodiscard]] uint8_t read(uint16_t addr) const ;


        void write(uint16_t addr, const uint8_t &data);


    };
};


#endif //MMU_H
