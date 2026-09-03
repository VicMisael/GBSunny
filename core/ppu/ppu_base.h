#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include "shared/hardware_constants.h"
#include "types.h"

class PPU_Base {
public:

	virtual ~PPU_Base() = default;

	// Lifecycle
	virtual void reset() = 0;
	virtual void step(uint32_t cycles) = 0;
	virtual void tick() = 0;

	// VRAM access
	[[nodiscard]] virtual uint8_t  read_vram(uint16_t address) const = 0;
	virtual void write_vram(uint16_t address, uint8_t value) = 0;
	[[nodiscard]] virtual const uint8_t* get_vram_ptr() const = 0;

	// OAM access
	[[nodiscard]] uint8_t read_oam(uint16_t addr) const {
		if (!is_oam_accessible()) return 0xFF;
		return oam[addr - 0xFE00];
	}

	void write_oam(uint16_t addr, uint8_t data) {
		oam[addr - 0xFE00] = data;
	}

	// Control register access
	[[nodiscard]] virtual uint8_t read_control(uint16_t addr) const = 0;
	virtual void write_control(uint16_t addr, uint8_t data) = 0;

	// DMA handling
	virtual void start_dma_transfer() = 0;
	[[nodiscard]] virtual bool is_dma_active() const = 0;

	// Access checks
	[[nodiscard]] virtual bool is_vram_accessible() const = 0;
	[[nodiscard]] virtual bool is_oam_accessible() const = 0;

	void set_vram_access_callback(std::function<void(bool)> callback) {
		vram_access_changed_callback = std::move(callback);
	}

	void set_dma_callback(std::function<void(bool)> callback) {
		dma_changed_callback = std::move(callback);
	}

	// Framebuffer access
	[[nodiscard]] virtual const std::array<ppu_types::rgba, gb_hardware::display::PixelCount>& get_framebuffer() const = 0;
protected:

	void notify_vram_access_changed(bool isAccessible) const
	{
		if (vram_access_changed_callback) {
			vram_access_changed_callback(isAccessible);
		}
	}

	void notify_dma_changed(bool isActive) const
	{
		if (dma_changed_callback) {
			dma_changed_callback(isActive);
		}
	}

	// PPU Registers using the types from ppu_types.h
	PPU_Base() :lcdc(0)
	{

	}

	static_assert(sizeof(ppu_types::OAM_Sprite) == 4);
	static_assert(sizeof(std::array<uint8_t, 160>) == 160);
	static_assert(sizeof(std::array<ppu_types::OAM_Sprite, 40>) == 160);

	union {
		std::array<uint8_t, 160> oam{};
		std::array<ppu_types::OAM_Sprite, 40> oam_sprites;
	};

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
	std::function<void(bool)> vram_access_changed_callback{};
	std::function<void(bool)> dma_changed_callback{};

	const std::array<ppu_types::rgba, 4> colors = {
			ppu_types::rgba{.value = 0xFFFFFFFF}, // White
			ppu_types::rgba{.value = 0xC0C0C0FF}, // Light gray
			ppu_types::rgba{.value = 0x606060FF}, // Dark gray
			ppu_types::rgba{.value = 0x000000FF}  // Black
	}
	;

};
