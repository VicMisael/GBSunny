//
// Created by visael on 14/03/25.
//

#ifndef TIMER2_H
#define TIMER2_H
#include <cstdint>
#include <memory>

#include "shared/interrupt.h"

#include "base_timer.h"
//BETTER
class gb_timer2:public base_timer {
    public:
    gb_timer2( const std::shared_ptr<shared::interrupt>& interrupt_controller):interrupt_controller(interrupt_controller){};
    [[nodiscard]] uint8_t read(uint16_t addr) const override;
    void write(uint16_t addr, uint8_t data) override;

    void reset();
    void step(uint32_t cycles) override;


private:
    bool last_cycle_and_result = false;
    bool tima_reload_pending = false;
    uint8_t tima_reload_delay = 0;
    void tick();
    std::shared_ptr<shared::interrupt> interrupt_controller;

    uint16_t div_reg; 

    uint8_t tima_reg = 0; // 0xFF05 - TIMA (Timer Counter)
    uint8_t tma_reg = 0;  // 0xFF06 - TMA (Timer Modulo)
    uint8_t tac_reg = 0;  // 0xFF07 - TAC (Timer Control)
};



#endif //TIMER2_H
