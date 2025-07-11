//
// Created by Misael on 19/03/2025.
//

#pragma once

#include <cstdint>

namespace ppu_types{

    enum ppu_mode {
        HBLANK = 0,
        VBLANK = 1,
        OAM_SCAN = 2,
        DRAWING = 3
    };
    union rgba{
        uint32_t value{};
    struct  {
        uint8_t A;
        uint8_t B;
        uint8_t G;
        uint8_t R;
    };
    };


    struct OAM_Sprite {
        uint8_t y;          // Byte 0: Y-coordinate
        uint8_t x;          // Byte 1: X-coordinate
        uint8_t tile_index; // Byte 2: Tile pattern number
        uint8_t flags;      // Byte 3: Attributes (palette, flip, priority)
    };

    union _lcd_control {
        struct {
            uint8_t BG_window_enable:1;
            uint8_t OBJ_Enable:1;
            uint8_t OBJ_SIZE:1;
            uint8_t BG_tile_map:1;
            uint8_t BG_window_tiles:1;
            uint8_t window_enable:1;
            uint8_t window_tile_map_area:1;
            uint8_t LCD_PPU_enable:1;
        } bits;
        uint8_t data;

        // Constructor to allow initialization from a byte
        explicit _lcd_control(uint8_t val = 0) : data(val) {}
    } ;

    union _lcd_stat {
    public:
        struct {
            uint8_t ppu_mode:2;
            uint8_t LYC_eq_LY:1;
            uint8_t MODE_0_INT_SELECT:1; // H-Blank Interrupt
            uint8_t MODE_1_INT_SELECT:1; // V-Blank Interrupt
            uint8_t MODE_2_INT_SELECT:1; // OAM Interrupt
            uint8_t LYC_INT_SELECT:1;
            uint8_t unused:1;
        };
        uint8_t data = 0;

        void write(const uint8_t input) {
            // Corrected write logic: only bits 3-6 are writable.
            // The lower 3 bits (mode and LYC flag) are read-only.
            data = (input & 0b01111000) | (data & 0b10000111);
        };

        [[nodiscard]] uint8_t read() const {
            // Bit 7 is unused and always reads as 1.
            return data | 0b10000000;
        }
    };
}
