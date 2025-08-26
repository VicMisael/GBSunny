//
// Created by visael on 14/03/25.
//

#include "spu.h"

void spu::tick()
{
}

uint8_t spu::read(uint16_t addr)
{
    return 0;
}

uint8_t spu::read_wave(uint16_t addr)
{
    return 0;
}

void spu::write(uint16_t addr, uint8_t data)
{
}

void spu::write_wave(uint16_t addr, uint8_t data)
{
}

void spu::step(uint32_t cycles)
{
	for (uint32_t i = 0; i < cycles; i++) {
		//Run this tick by tick, step by step, might be slower than every option, Should be more accurate
		tick();
	}
}
