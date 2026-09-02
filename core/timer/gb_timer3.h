#ifndef GB_TIMER3_H
#define GB_TIMER3_H

#include <cstdint>
#include <memory>

#include "base_timer.h"
#include "shared/interrupt.h"

// Event-driven timer. DIV advances in batches and TIMA is updated only at
// falling edges of the selected divider bit.
class gb_timer3 final : public base_timer {
public:
    explicit gb_timer3(const std::shared_ptr<shared::interrupt>& interrupt_controller)
        : interrupt_controller(interrupt_controller) {}

    [[nodiscard]] uint8_t read(uint16_t addr) const override;
    void write(uint16_t addr, uint8_t data) override;
    void reset() override;
    void tick() override;
    void step(uint32_t cycles) override;

private:
    [[nodiscard]] bool timer_input() const;
    [[nodiscard]] uint32_t falling_edge_period() const;
    void timer_falling_edge();
    void advance_reload(uint32_t cycles);

    std::shared_ptr<shared::interrupt> interrupt_controller;
    uint16_t div_reg = 0;
    uint8_t tima_reg = 0;
    uint8_t tma_reg = 0;
    uint8_t tac_reg = 0;
    bool tima_reload_pending = false;
    uint8_t tima_reload_delay = 0;
};

#endif // GB_TIMER3_H
