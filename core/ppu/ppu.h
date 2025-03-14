//
// Created by Misael on 12/03/2025.
//

#ifndef PPU_H
#define PPU_H
#include <cstdint>


class PPU {
    uint8_t video_RAM[8192] = {};

public:
    const uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
};



#endif //PPU_H
