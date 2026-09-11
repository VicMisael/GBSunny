#include "gb_timer3.h"

#include <algorithm>

namespace {
constexpr uint8_t div_bits[4] = {9, 3, 5, 7};
}

void gb_timer3::reset() {
    div_reg = 0;
    tima_reg = 0;
    tma_reg = 0;
    tac_reg = 0;
    tima_reload_pending = false;
    tima_reload_delay = 0;
}

bool gb_timer3::timer_input() const {
    const uint8_t bit = div_bits[tac_reg & 0x03];
    return (tac_reg & 0x04) != 0 && ((div_reg >> bit) & 1) != 0;
}

uint32_t gb_timer3::falling_edge_period() const {
    return 1u << (div_bits[tac_reg & 0x03] + 1);
}

void gb_timer3::advance_reload(uint32_t cycles) {
    if (!tima_reload_pending) {
        return;
    }

    if (cycles < tima_reload_delay) {
        tima_reload_delay -= static_cast<uint8_t>(cycles);
        return;
    }

    tima_reload_delay = 0;
    tima_reload_pending = false;
    tima_reg = tma_reg;
    interrupt_controller->requested.timer = true;
}

void gb_timer3::timer_falling_edge() {
    if (tima_reload_pending) {
        return;
    }

    if (tima_reg == 0xFF) {
        tima_reg = 0;
        tima_reload_pending = true;
        tima_reload_delay = 4;
    }
    else {
        ++tima_reg;
    }
}

void gb_timer3::tick() {
    step(1);
}

void gb_timer3::step(uint32_t cycles) {
    while (cycles != 0) {
        if ((tac_reg & 0x04) == 0) {
            advance_reload(cycles);
            div_reg = static_cast<uint16_t>(div_reg + cycles);
            return;
        }

        const uint32_t period = falling_edge_period();
        const uint32_t cycles_to_edge = period - (div_reg & (period - 1));
        const uint32_t advance = std::min(cycles, cycles_to_edge);

        advance_reload(advance);
        div_reg = static_cast<uint16_t>(div_reg + advance);
        cycles -= advance;

        if (advance == cycles_to_edge) {
            timer_falling_edge();
        }
    }
}

uint8_t gb_timer3::read(uint16_t addr) const {
    switch (addr) {
    case 0xFF04: return static_cast<uint8_t>(div_reg >> 8);
    case 0xFF05: return tima_reg;
    case 0xFF06: return tma_reg;
    case 0xFF07: return tac_reg;
    default: return 0xFF;
    }
}

void gb_timer3::write(uint16_t addr, uint8_t data) {
    switch (addr) {
    case 0xFF04: {
        const bool old_input = timer_input();
        div_reg = 0;
        if (old_input && !timer_input()) {
            timer_falling_edge();
        }
        break;
    }
    case 0xFF05:
        tima_reg = data;
        tima_reload_pending = false;
        tima_reload_delay = 0;
        break;
    case 0xFF06:
        tma_reg = data;
        break;
    case 0xFF07: {
        const bool old_input = timer_input();
        tac_reg = data & 0x07;
        if (old_input && !timer_input()) {
            timer_falling_edge();
        }
        break;
    }
    default:
        break;
    }
}
