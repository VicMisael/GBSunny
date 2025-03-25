//
// Created by Misael on 12/03/2025.
//

#include "ppu.h"

void PPU::start_oam_transfer() {
	oam_transfer_remaining_cycles = 160;
}

bool PPU::is_oam_transfer_running() const {
	return oam_transfer_remaining_cycles > 0;
}

void PPU::reset()
{
}

uint8_t PPU::read(uint16_t address) const {
	return 0xff;
}

void PPU::write(uint16_t address, uint8_t value)
{
}

uint8_t PPU::read_oam(uint16_t addr) const
{
	return 0;
}

uint8_t PPU::read_ppucontrol(uint16_t addr) const {
	switch (addr&0x00ff) {
		case 0x40:
			return lcd_control.data;
		case 0x41: 
		default:
			return 0xff;
	}
	return 0;
}

void PPU::write_ppucontrol(uint16_t addr, uint8_t data) {
}

void PPU::write_oam(uint16_t addr, uint8_t data) {
}

void PPU::step(const uint8_t cycles_to_run) {
}
