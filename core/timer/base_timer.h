#pragma once

class base_timer
{
public:
	virtual  uint8_t read(uint16_t addr) const = 0;
	virtual void write(uint16_t addr, uint8_t data) = 0;
	virtual void reset() = 0;
	virtual void step(uint32_t cycles) = 0;

};