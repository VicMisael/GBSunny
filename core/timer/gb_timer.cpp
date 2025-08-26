//
// Created by visael on 14/03/25.
//

#include "gb_timer.h"
void gb_timer::reset() {
    div_reg = 0;
    tima_reg = 0;
    tma_reg = 0;
    tac_reg = 0;
    div_counter = 0;
    tima_counter = 0;
}

void gb_timer::step(uint32_t cycles) {
    // 1. Handle the DIV register. It increments at a fixed rate of 16384 Hz,
    // which is equivalent to every 256 T-cycles (4194304 / 16384 = 256).
    div_counter += cycles;
    if (div_counter >= 256) {
        div_counter -= 256;
        div_reg++; // This will naturally wrap around on overflow
    }

    // 2. Check if the timer is enabled in the TAC register (bit 2).
    bool timer_enabled = (tac_reg & 0b100) != 0;
    if (!timer_enabled) {
        return;
    }

    // 3. Handle the TIMA register.
    tima_counter += cycles;
    uint32_t threshold = get_tima_threshold();

    // It's possible for TIMA to increment multiple times in one step if many cycles passed.
    while (tima_counter >= threshold) {
        tima_counter -= threshold;

        if (tima_reg == 0xFF) {
            // TIMA overflows. Reset it to the value in TMA.
            tima_reg = tma_reg;
            // Request a timer interrupt.
            interrupt_controller->requested.timer = true;
        } else {
            tima_reg++;
        }
    }
}

uint32_t gb_timer::get_tima_threshold() const {
    // Bits 1-0 of TAC select the frequency.
    uint8_t frequency_select = tac_reg & 0b11;
    switch (frequency_select) {
        case 0: return 1024; // 4096 Hz   (4194304 / 4096)
        case 1: return 16;   // 262144 Hz (4194304 / 262144)
        case 2: return 64;   // 65536 Hz  (4194304 / 65536)
        case 3: return 256;  // 16384 Hz  (4194304 / 16384)
        default: return 1024; // Should not happen
    }
}

uint8_t gb_timer::read(uint16_t addr) const {
    switch (addr) {
        case 0xFF04: return div_reg;
        case 0xFF05: return tima_reg;
        case 0xFF06: return tma_reg;
        case 0xFF07: return tac_reg;
        default: return 0xFF; // Should not be called with other addresses
    }
}

void gb_timer::write(uint16_t addr, uint8_t data) {
    switch (addr) {
        case 0xFF04:
            // Any write to DIV resets it to 0.
            div_reg = 0;
            div_counter = 0; // Also reset the internal counter
            break;
        case 0xFF05:
            tima_reg = data;
            break;
        case 0xFF06:
            tma_reg = data;
            break;
        case 0xFF07:
            tac_reg = data;
            break;
    }
}