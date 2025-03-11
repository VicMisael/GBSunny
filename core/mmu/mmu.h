//
// Created by Misael on 08/03/2025.
//

#ifndef MMU_H
#define MMU_H
#include <cstdint>

namespace mmu {


class mmu {
    uint8_t internal_RAM[8192] = {};
    uint8_t video_RAM[8192] = {};
    uint16_t offset = 0x1000;
    uint8_t temp = 0;
    public:
    uint8_t& read(uint16_t addr);
    void write(uint16_t addr,const uint8_t& data);

};
    };



#endif //MMU_H
