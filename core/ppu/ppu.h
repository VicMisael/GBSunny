//
// Created by Misael on 12/03/2025.
//

#ifndef PPU_H
#define PPU_H
#include "../shared/interrupt.h"
#include <cstdint>
#include <memory>



class PPU {
    uint8_t video_RAM[8192] = {};
    std::shared_ptr<shared::interrupt> interrupt; //Shared space for interrupts

public:
    void reset();
    const uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);
    uint8_t read_oam(uint16_t addr) const;
    uint8_t read_ppucontrol(uint16_t addr) const;
    void write_ppucontrol(uint16_t addr, uint8_t data);
    void write_oam(uint16_t addr, uint8_t data);
};



#endif //PPU_H
