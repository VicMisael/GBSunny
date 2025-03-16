//
// Created by visael on 14/03/25.
//

#ifndef SPU_H
#define SPU_H
#include <cstdint>
#include <shared/interrupt.h>
#include <memory>


class spu {
    std::shared_ptr<shared::interrupt> interrupts;
public:
    uint8_t read(uint16_t addr);
    uint8_t read_wave(uint16_t addr);
    void write(uint16_t addr,uint8_t data);
    void write_wave(uint16_t addr, uint8_t data);
};



#endif //SPU_H
