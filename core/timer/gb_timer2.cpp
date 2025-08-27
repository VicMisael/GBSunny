//
// Created by visael on 14/03/25.
//

#include "gb_timer2.h"

void gb_timer2::reset() {
	div_reg = 0;
	tima_reload_delay = 0;
	last_cycle_and_result = false;
}

void gb_timer2::step(uint32_t cycles) {
	for (uint32_t i = 0; i < cycles; i++) {
		//Run this tick by tick, step by step, might be slower than every option, Should be more accurate
		tick();
	}
}

const uint8_t div_bit[4] = { 9, 3, 5, 7 };
void gb_timer2::tick()
{
	div_reg++;

	const uint8_t bit = div_bit[tac_reg & 0b11];
	const bool divbit = (div_reg >> bit) & 0b1;
	const bool tac_enable = tac_reg & 0x4;
	const bool andResult = divbit && tac_enable;
	if(last_cycle_and_result && !andResult)
	{

			if (tima_reg == 0xFF) {
				tima_reg = 0x00;
				// Start reload sequence
				tima_reload_pending = true;
				tima_reload_delay = 4;
				interrupt_controller->requested.timer = true;
			}
			else {
				tima_reg++;
			}
		

		// Handle delayed reload
		if (tima_reload_pending) {
			if (--tima_reload_delay == 0) {
				tima_reg = tma_reg;
				tima_reload_pending = false;
			}
		}

		last_cycle_and_result = andResult;
		
	}


	last_cycle_and_result = andResult;

}

uint8_t gb_timer2::read(uint16_t addr) const {
	switch (addr) {
	case 0xFF04: return(div_reg >> 8);
	case 0xFF05: return tima_reg;
	case 0xFF06: return tma_reg;
	case 0xFF07: return tac_reg;
	default: return 0xFF; // Should not be called with other addresses
	}
}

void gb_timer2::write(uint16_t addr, uint8_t data) {
	switch (addr) {
	case 0xFF04:
		// Any write to DIV resets it to 0.
		div_reg = 0;
		break;
	case 0xFF05:
		tima_reg = data;
		tima_reload_pending = false;
		break;
	case 0xFF06:
		tma_reg = data;
		break;
	case 0xFF07:
		tac_reg = data & 0x07;
		break;
	}
}