//
// Created by Misael on 12/03/2025.
//

#include "ppu.h"

void PPU::reset()
{
}

const uint8_t PPU::read(uint16_t address) const
{
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
	return 0;
}

void PPU::write_ppucontrol(uint16_t addr, uint8_t data) {
}

void PPU::write_oam(uint16_t addr, uint8_t data) {
}
