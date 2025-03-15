//
// Created by Misael on 12/03/2025.
//

#ifndef PPU_H
#define PPU_H
#include <cstdint>


class PPU {
    uint8_t video_RAM[8192] = {};

public:
    void reset();
    const uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);
    uint8_t read_oam(uint16_t addr);
    uint8_t read_ppucontrol(uint16_t addr);
    void write_oam(uint16_t addr, uint8_t data);
};



#endif //PPU_H
