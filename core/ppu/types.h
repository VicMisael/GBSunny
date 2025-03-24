//
// Created by Misael on 19/03/2025.
//

#pragma once

#include <cstdint>

namespace ppu_types{

    enum ppu_mode {
        OAM,
        VRAM,
        HBLANK,
        VBLANK

    };
    union _lcd_control {
        struct {
            bool BG_window_enable:1;
            bool OBJ_Enable:1;
            bool OBJ_SIZE:1;
            bool BG_tile_map:1;
            bool BG_window_tiles:1;
            bool window_enable:1;
            bool window_tile_map_are:1;
            bool LCD_PPU_enable:1;
        } bits;
        uint8_t data;
        explicit _lcd_control(uint8_t data) : data(data) {}
    } ;

}
