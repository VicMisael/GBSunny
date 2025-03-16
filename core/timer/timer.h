//
// Created by visael on 14/03/25.
//

#ifndef TIMER_H
#define TIMER_H
#include <cstdint>


class timer {
    public:
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
};



#endif //TIMER_H
