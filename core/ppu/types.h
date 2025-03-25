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

    union _lcd_stat {

    private:
        uint8_t data = 0;
    public:
        struct {
            uint8_t ppumode:2;
            bool LYC_eq_LY:1;
            bool MODE_0_INT_SELECT:1;
            bool MODE_1_INT_SELECT:1;
            bool MODE_2_INT_SELECT:1;
            bool LYC_INT_SELECT:1;
            bool :1;
        };


        void write(const uint8_t input) {
            data = input & 0x7E;
        };

        [[nodiscard]] uint8_t read() const {
            return data;
        }

    };

}
