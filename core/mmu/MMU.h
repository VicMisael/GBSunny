//
// Created by Misael on 08/03/2025.
//

#ifndef MMU_H
#define MMU_H
#include <cstdint>
#include <memory>

#include "cartridge/cartridge.h"
#include "ppu/ppu.h"

namespace mmu {
    class MMU {
        uint8_t internal_RAM[8192] = {};
        PPU _ppu;
        uint16_t offset = 0x1000;
        uint8_t temp = 0;
        std::shared_ptr<Cartridge> cartridge;

    public:
        MMU(const std::shared_ptr<Cartridge> &cartridge, const PPU &ppu): cartridge(cartridge), _ppu(ppu) {
        };

        uint8_t read(uint16_t addr);

        uint8_t &read_as_ref(uint16_t addr);

        void write(uint16_t addr, const uint8_t &data);
    };
};


#endif //MMU_H
