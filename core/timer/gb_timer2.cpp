//
// Created by visael on 14/03/25.
//

#include "gb_timer2.h"

constexpr static uint8_t div_bit[4] = {
    9, 3, 5, 7
};

void gb_timer2::reset()
{
    div_reg = 0;

    tima_reg = 0;
    tma_reg = 0;
    tac_reg = 0;

    tima_reload_pending = false;
    tima_reload_delay = 0;
}

bool gb_timer2::timer_signal() const
{
    // TAC bit 2 = timer enable
    if ((tac_reg & 0x04) == 0)
        return false;

    const uint8_t bit = div_bit[tac_reg & 0x03];

    return ((div_reg >> bit) & 1) != 0;
}

void gb_timer2::increment_tima()
{
    //
    // During the overflow delay TIMA isn't supposed to generate
    // another overflow sequence.
    //
    if (tima_reload_pending)
        return;

    if (tima_reg == 0xFF) {
        //
        // Overflow.
        //
        // TIMA becomes 00 immediately, then TMA is copied into TIMA
        // after 4 T-cycles.
        //
        tima_reg = 0x00;

        tima_reload_pending = true;
        tima_reload_delay = 4;
    }
    else {
        ++tima_reg;
    }
}

void gb_timer2::step(uint32_t cycles)
{
    for (uint32_t i = 0; i < cycles; ++i) {


        if (tima_reload_pending) {
            --tima_reload_delay;

            if (tima_reload_delay == 0) {
                tima_reg = tma_reg;

                tima_reload_pending = false;

                interrupt_controller->requested.timer = true;
            }
        }


        const bool old_signal = timer_signal();

        ++div_reg;

        const bool new_signal = timer_signal();

        if (old_signal && !new_signal) {
            increment_tima();
        }
    }
}

void gb_timer2::tick()
{
    step(1);
}

uint8_t gb_timer2::read(uint16_t addr) const
{
    switch (addr) {
        case 0xFF04:
            // DIV exposes the upper 8 bits of the internal divider.
            return static_cast<uint8_t>(div_reg >> 8);

        case 0xFF05:
            return tima_reg;

        case 0xFF06:
            return tma_reg;

        case 0xFF07:
            /*
             * Only bits 0-2 are writable.
             *
             * On real hardware the unused bits read as 1, so FF07
             * normally reads:
             *
             *     11111xxx
             */
            return tac_reg | 0xF8;

        default:
            return 0xFF;
    }
}

void gb_timer2::write(uint16_t addr, uint8_t data)
{
    switch (addr) {

        /*
         * DIV
         */
        case 0xFF04: {
            /*
             * Writing any value to DIV resets the internal divider.
             *
             * Importantly, resetting DIV may change:
             *
             *     timer_signal: 1 -> 0
             *
             * which causes TIMA to increment.
             */
            const bool old_signal = timer_signal();

            div_reg = 0;

            const bool new_signal = timer_signal();

            if (old_signal && !new_signal) {
                increment_tima();
            }

            break;
        }

        /*
         * TIMA
         */
        case 0xFF05: {
            /*
             * Writing TIMA during the overflow delay cancels the
             * pending reload.
             *
             * NOTE:
             * There is an additional hardware quirk concerning a
             * write during the exact reload T-cycle. If you want
             * maximum Mooneye accuracy, model the reload cycle as a
             * separate state.
             */
            tima_reg = data;

            tima_reload_pending = false;
            tima_reload_delay = 0;

            break;
        }

        /*
         * TMA
         */
        case 0xFF06: {
            tma_reg = data;

            break;
        }

        /*
         * TAC
         */
        case 0xFF07: {
            const bool old_signal = timer_signal();

            tac_reg = data & 0x07;

            const bool new_signal = timer_signal();

            if (old_signal && !new_signal) {
                increment_tima();
            }

            break;
        }

        default:
            break;
    }
}