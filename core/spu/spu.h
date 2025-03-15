//
// Created by visael on 14/03/25.
//

#ifndef SPU_H
#define SPU_H
#include <bits/stdint-uintn.h>


class spu {
public:
    uint8_t read(uint16_t addr);
    uint8_t read_wave(uint16_t addr);
};



#endif //SPU_H
