#pragma once

#include <memory>
#include <array>
#include <cstdint>
#include "types.h"

class PPU_Base {
public:
	virtual ~PPU_Base() = default;

	// Lifecycle
	virtual void reset() = 0;
	virtual void step(uint32_t cycles) = 0;

	// VRAM access
	[[nodiscard]] virtual uint8_t read_vram(uint16_t address) const = 0;
	virtual void write_vram(uint16_t address, uint8_t value) = 0;

	// OAM access
	[[nodiscard]] virtual uint8_t read_oam(uint16_t addr) const = 0;
	virtual void write_oam(uint16_t addr, uint8_t data) = 0;

	// Control register access
	[[nodiscard]] virtual uint8_t read_control(uint16_t addr) const = 0;
	virtual void write_control(uint16_t addr, uint8_t data) = 0;

	// DMA handling
	virtual void start_dma_transfer() = 0;
	[[nodiscard]] virtual bool is_dma_active() const = 0;

	// Access checks
	[[nodiscard]] virtual bool is_vram_accessible() const = 0;
	[[nodiscard]] virtual bool is_oam_accessible() const = 0;

	// Framebuffer access
	[[nodiscard]] virtual const std::array<ppu_types::rgba, 160 * 144>& get_framebuffer() const = 0;
protected:
	// PPU Registers using the types from ppu_types.h
	PPU_Base() :lcdc(0)
	{

	}
	ppu_types::_lcd_control lcdc;
	ppu_types::_lcd_stat stat;
	uint8_t scy{};
	uint8_t scx{};
	uint8_t ly{};
	uint8_t lyc{};
	uint8_t bgp{};
	uint8_t obp0{};
	uint8_t obp1{};
	uint8_t wy{};
	uint8_t wx{};
	int32_t dma_cycles_remaining = 0;
	const std::array<ppu_types::rgba, 4> colors = {
			0xFFFFFFFF, // White
			0xC0C0C0FF, // Light gray
			0x606060FF, // Dark gray
			0x000000FF  // Black
	}
	;

};