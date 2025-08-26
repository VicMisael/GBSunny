//
// Created by Misael on 19/03/2025.
//

#pragma once

#include <cstdint>
#include <array>

namespace ppu_types {

	enum ppu_mode {
		HBLANK = 0,
		VBLANK = 1,
		OAM_SCAN = 2,
		DRAWING = 3
	};


	union rgba {
		uint32_t value{};
		struct {
			uint8_t A;
			uint8_t B;
			uint8_t G;
			uint8_t R;
		};
	};

	struct line {
		uint8_t lsb;
		uint8_t msb;
		std::array<uint8_t, 8> decoded_pixels(bool x_flip = false) const {
			std::array<uint8_t, 8> pixels{};

			for (int i = 7; i >= 0; --i) {
				uint8_t hi = (msb >> i) & 1;
				uint8_t lo = (lsb >> i) & 1;
				uint8_t color = (hi << 1) | lo;

				int idx = 7 - i;             // normal index
				if (x_flip) idx = i;         // reversed
				pixels[idx] = color;
			}
			return pixels;
		}

		uint16_t decoded_pixels_packed() const {
			uint16_t packed = 0;

			for (int i = 7; i >= 0; --i) {
				uint8_t hi = (msb >> i) & 1;
				uint8_t lo = (lsb >> i) & 1;
				uint8_t pixel = (hi << 1) | lo;

				packed <<= 2;      // shift left to make room for next 2-bit pixel
				packed |= pixel;   // append the 2-bit pixel
			}

			return packed;
		}
	};

	struct tile {
		union {
			line lines[8];
			uint8_t bytes[16];
		};
	};

	union oam_sprite_flags {
		
		struct {
			uint8_t cgb_placeholder : 3;
			bool palette_number : 1;
			bool x_flip : 1;
			bool y_flip : 1;
			bool obj_to_dbg_priority:1;
		};
		uint8_t byte;
	};
	
	struct pixel_info{
		uint8_t color : 2;
		uint8_t palette : 2;
		uint8_t priority; //CGB;
		uint8_t bg_priority;
	};
	struct OAM_Sprite {
		uint8_t y;          // Byte 0: Y-coordinate
		uint8_t x;          // Byte 1: X-coordinate
		uint8_t tile_index; // Byte 2: Tile pattern number
		oam_sprite_flags flags;      // Byte 3: Attributes (palette, flip, priority)
	};

	union _lcd_control {
		struct {
			uint8_t BG_window_enable : 1; // BG Window Enable bit 0
			uint8_t OBJ_Enable : 1; // Sprite Enable bit 1
			uint8_t OBJ_SIZE : 1; // Sprite height 8 or 16 bit 2
			uint8_t BG_tile_map : 1; // Tile map select bit 3 Background MAP location
			uint8_t BG_window_tiles_adressing : 1; // tile addressing mode for bg window bit4
			uint8_t window_enable : 1; //bit 5  Setting this bit to 0 hides the window layer entirely.
			uint8_t window_tile_map_area : 1; //bit 6    If set to 1, the Window will use the background map located at $9C00-$9FFF. Otherwise, it uses $9800-$9BFF.
			uint8_t LCD_PPU_enable : 1; //bit7 Setting this bit to 0 disables the PPU entirely. The screen is turned off.
		} bits;

		struct {
			uint8_t bit0: 1; // BG Window Enable bit 0
			uint8_t bit1: 1; // Sprite Enable bit 1
			uint8_t bit2: 1; // Sprite height 8 or 16 bit 2
			uint8_t bit3: 1; //  If set to 1, the Background will use the background map located at $9C00-$9FFF. Otherwise, it uses $9800-$9BFF. bit 3
			uint8_t bit4: 1; //   If set to 1, fetching Tile Data uses the 8000 method. Otherwise, the 8800 method is used.bit4
			uint8_t bit5: 1; //bit 5  Setting this bit to 0 hides the window layer entirely.
			uint8_t bit6: 1; //bit 6    If set to 1, the Window will use the background map located at $9C00-$9FFF. Otherwise, it uses $9800-$9BFF.
			uint8_t bit7: 1; //bit7 Setting this bit to 0 disables the PPU entirely. The screen is turned off.
		} ;


		uint8_t data;

		// Constructor to allow initialization from a byte
		explicit _lcd_control(uint8_t val = 0) : data(val) {}
	};

	union _lcd_stat {
		struct {
			uint8_t ppu_mode : 2;
			bool LYC_eq_LY : 1;
			bool MODE_0_INT_SELECT : 1; // H-Blank Interrupt
			bool MODE_1_INT_SELECT : 1; // V-Blank Interrupt
			bool MODE_2_INT_SELECT : 1; // OAM Interrupt
			bool LYC_INT_SELECT : 1;
			bool unused : 1;
		};
		uint8_t data = 0;

		void write(const uint8_t input) {
			// Corrected write logic: only bits 3-6 are writable.
			// The lower 3 bits (mode and LYC requested) are read-only.
			data = (input & 0b01111000) | (data & 0b10000111);
		};

		[[nodiscard]] uint8_t read() const {
			// Bit 7 is unused and always reads as 1.
			return data | 0b10000000;
		}
	};
}
