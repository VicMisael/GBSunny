//
// Created by visael on 14/03/25.
//

#ifndef TIMER_H
#define TIMER_H
#include <cstdint>
#include <memory>

#include "base_timer.h"
#include "shared/interrupt.h"

//REALLY BAD
class gb_timer:public base_timer {
    public:
    gb_timer( const std::shared_ptr<shared::interrupt>& interrupt_controller):interrupt_controller(interrupt_controller){};
    [[nodiscard]] uint8_t read(uint16_t addr) const;
    void write(uint16_t addr, uint8_t data);

    void reset();
    void step(uint32_t cycles);


private:
    [[nodiscard]] uint32_t get_tima_threshold() const;

    // Direct handle to the central interrupt controller
    std::shared_ptr<shared::interrupt> interrupt_controller;

    // Timer Registers
    uint8_t div_reg = 0;  // 0xFF04 - DIV (Divider Register)
    uint8_t tima_reg = 0; // 0xFF05 - TIMA (Timer Counter)
    uint8_t tma_reg = 0;  // 0xFF06 - TMA (Timer Modulo)
    uint8_t tac_reg = 0;  // 0xFF07 - TAC (Timer Control)

    // Internal counters for tracking cycles
    uint32_t div_counter = 0;
    uint32_t tima_counter = 0;
};



#endif //TIMER_H
