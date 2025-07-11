/*
 * =====================================================================================
 *
 * Filename:  ppu.cpp
 *
 * =====================================================================================
 */

#include "ppu.h"
#include <algorithm>
#include <vector>

// PPU timings in T-cycles
constexpr int OAM_SCAN_CYCLES = 80;
constexpr int DRAWING_CYCLES = 172;
constexpr int HBLANK_CYCLES = 204;
constexpr int SCANLINE_CYCLES = 456;
constexpr int VBLANK_LINES = 10;
constexpr int FRAME_LINES = 154;

PPU::PPU(std::shared_ptr<shared::interrupt> interrupt_controller)
    : interrupt_controller(std::move(interrupt_controller)),
      lcdc(0),
      colors{{
          { .value = 0xFFFFFFFF }, // White
          { .value = 0xFFAAAAAA }, // Light Grey
          { .value = 0xFF555555 }, // Dark Grey
          { .value = 0xFF000000 }  // Black
      }} {

    reset();
}

void PPU::reset() {
    lcdc.data = 0xf;
    stat.data = 0;
    scy = 0;
    scx = 0;
    ly = 0;
    lyc = 0;
    bgp = 0xFC;
    obp0 = 0xFF;
    obp1 = 0xFF;
    wy = 0;
    wx = 0;
    cycle_counter = 0;
    dma_cycles_remaining = 0;
    window_line_counter = 0;
    set_mode(ppu_types::OAM_SCAN);

    std::fill(std::begin(vram), std::end(vram), 0xff);
    std::fill(std::begin(oam), std::end(oam), 0xff);
}

void PPU::step(uint32_t cycles_to_run) {
    if (!lcdc.bits.LCD_PPU_enable) {
        cycle_counter = 0;
        ly = 0;
        set_mode(ppu_types::HBLANK);
        return;
    }

    if (dma_cycles_remaining > 0) {
        dma_cycles_remaining -= static_cast<int32_t>(cycles_to_run);
        if (dma_cycles_remaining < 0) dma_cycles_remaining = 0;
    }

    cycle_counter += cycles_to_run;

    switch (current_mode) {
        case ppu_types::OAM_SCAN:
            if (cycle_counter >= OAM_SCAN_CYCLES) {
                cycle_counter -= OAM_SCAN_CYCLES;
                set_mode(ppu_types::DRAWING);
            }
            break;
        case ppu_types::DRAWING:
            if (cycle_counter >= DRAWING_CYCLES) {
                cycle_counter -= DRAWING_CYCLES;

                render_scanline();
                set_mode(ppu_types::HBLANK);
            }
            break;
        case ppu_types::HBLANK:
            if (cycle_counter >= HBLANK_CYCLES) {
                cycle_counter -= HBLANK_CYCLES;
                increment_ly();
                if (ly == 144) {
                    set_mode(ppu_types::VBLANK);
                    // Correctly set the VBlank interrupt flag
                    interrupt_controller->flag.VBlank = true;
                } else {
                    set_mode(ppu_types::OAM_SCAN);
                }
            }
            break;
        case ppu_types::VBLANK:
            if (cycle_counter >= SCANLINE_CYCLES) {
                cycle_counter -= SCANLINE_CYCLES;
                increment_ly();
                if (ly >= FRAME_LINES) {
                    ly = 0;
                    window_line_counter = 0;
                    set_mode(ppu_types::OAM_SCAN);
                }
            }
            break;
    }
}

void PPU::increment_ly() {
    ly++;
    check_lyc_coincidence();
}

void PPU::check_lyc_coincidence() {
    stat.LYC_eq_LY = (ly == lyc);
    if (stat.LYC_eq_LY && stat.LYC_INT_SELECT) {
        interrupt_controller->flag.LCD = true;
    }
}

void PPU::set_mode(ppu_types::ppu_mode new_mode) {
    current_mode = new_mode;
    stat.ppu_mode = new_mode;

    bool interrupt_requested = false;
    switch (new_mode) {
        case ppu_types::HBLANK:   interrupt_requested = stat.MODE_0_INT_SELECT; break;
        case ppu_types::VBLANK:   interrupt_requested = stat.MODE_1_INT_SELECT; break;
        case ppu_types::OAM_SCAN: interrupt_requested = stat.MODE_2_INT_SELECT; break;
        case ppu_types::DRAWING:  break;
    }

    if (interrupt_requested) {
        interrupt_controller->flag.LCD = true;
    }
}

void PPU::start_dma_transfer() {
    dma_cycles_remaining = 640;
}

bool PPU::is_dma_active() const {
    return dma_cycles_remaining > 0;
}

bool PPU::is_vram_accessible() const {
    return current_mode != ppu_types::DRAWING && dma_cycles_remaining == 0;
}

bool PPU::is_oam_accessible() const {
    return current_mode != ppu_types::OAM_SCAN && current_mode != ppu_types::DRAWING;
}

const std::array<ppu_types::rgba, 160 * 144>& PPU::get_framebuffer() const {
    return framebuffer;
}

ppu_types::rgba PPU::get_color_from_palette(uint8_t color_id, uint8_t palette_reg) const {
    int shade_index = (palette_reg >> (color_id * 2)) & 0b11;
    return colors[shade_index];
}

void PPU::render_scanline() {
    // This logic remains the same
    if (lcdc.bits.BG_window_enable) {
        render_background();
        render_window();
    }
    if (lcdc.bits.OBJ_Enable) {
        render_sprites();
    }
}

void PPU::render_background() {
    uint16_t tile_data_area = lcdc.bits.BG_window_tiles ? 0x8000 : 0x8800;
    uint16_t tile_map_area = lcdc.bits.BG_tile_map ? 0x9C00 : 0x9800;
    uint8_t y_in_map = scy + ly;
    uint8_t tile_row = y_in_map / 8;

    for (int pixel = 0; pixel < 160; ++pixel) {
        uint8_t x_in_map = scx + pixel;
        uint8_t tile_col = x_in_map / 8;

        uint16_t tile_map_addr = tile_map_area + tile_row * 32 + tile_col;
        uint8_t tile_id = vram[tile_map_addr - 0x8000];

        uint16_t tile_data_addr;
        if (lcdc.bits.BG_window_tiles) {
            tile_data_addr = tile_data_area + tile_id * 16;
        } else {
            tile_data_addr = tile_data_area + (static_cast<int8_t>(tile_id) + 128) * 16;
        }

        uint8_t y_in_tile = y_in_map % 8;
        uint8_t byte1 = vram[tile_data_addr + y_in_tile * 2 - 0x8000];
        uint8_t byte2 = vram[tile_data_addr + y_in_tile * 2 + 1 - 0x8000];
        uint8_t x_in_tile = 7 - (x_in_map % 8);
        uint8_t color_id = (((byte2 >> x_in_tile) & 1) << 1) | ((byte1 >> x_in_tile) & 1);

        scanline_buffer[pixel] = color_id;
        framebuffer[ly * 160 + pixel] = get_color_from_palette(color_id, bgp);
    }
}

void PPU::render_window() {
    if (!lcdc.bits.window_enable || ly < wy) return;

    uint16_t tile_data_area = lcdc.bits.BG_window_tiles ? 0x8000 : 0x8800;
    uint16_t tile_map_area = lcdc.bits.window_tile_map_area ? 0x9C00 : 0x9800;
    uint8_t y_in_map = window_line_counter;
    uint8_t tile_row = y_in_map / 8;

    for (int pixel = 0; pixel < 160; ++pixel) {
        if (pixel < (wx - 7)) continue;

        uint8_t x_in_map = pixel - (wx - 7);
        uint8_t tile_col = x_in_map / 8;

        uint16_t tile_map_addr = tile_map_area + tile_row * 32 + tile_col;
        uint8_t tile_id = vram[tile_map_addr - 0x8000];

        uint16_t tile_data_addr;
        if (lcdc.bits.BG_window_tiles) {
            tile_data_addr = tile_data_area + tile_id * 16;
        } else {
            tile_data_addr = tile_data_area + (static_cast<int8_t>(tile_id) + 128) * 16;
        }

        uint8_t y_in_tile = y_in_map % 8;
        uint8_t byte1 = vram[tile_data_addr + y_in_tile * 2 - 0x8000];
        uint8_t byte2 = vram[tile_data_addr + y_in_tile * 2 + 1 - 0x8000];
        uint8_t x_in_tile = 7 - (x_in_map % 8);
        uint8_t color_id = (((byte2 >> x_in_tile) & 1) << 1) | ((byte1 >> x_in_tile) & 1);

        scanline_buffer[pixel] = color_id;
        framebuffer[ly * 160 + pixel] = get_color_from_palette(color_id, bgp);
    }
    window_line_counter++;
}

void PPU::render_sprites() {
    uint8_t sprite_height = lcdc.bits.OBJ_SIZE ? 16 : 8;
    std::vector<ppu_types::OAM_Sprite> visible_sprites(11);

    for (int i = 0; i < 40; ++i) {
        if (visible_sprites.size() >= 10) break;

        ppu_types::OAM_Sprite sprite{};
        sprite.y = oam[i * 4 + 0];
        sprite.x = oam[i * 4 + 1];
        sprite.tile_index = oam[i * 4 + 2];
        sprite.flags = oam[i * 4 + 3];

        if (sprite.x > 0 && (ly + 16) >= sprite.y && (ly + 10) < (sprite.y + sprite_height)) {
            visible_sprites.push_back(sprite);
        }
    }

    std::sort(visible_sprites.begin(), visible_sprites.end(), [](const auto& a, const auto& b) {
        if (a.x != b.x) return a.x < b.x;
        return &a < &b; // Stable sort for sprites with same X
    });

    for (const auto& sprite : visible_sprites) {
        uint8_t palette_reg = (sprite.flags & (1 << 4)) ? obp1 : obp0;
        bool x_flip = (sprite.flags & (1 << 5));
        bool y_flip = (sprite.flags & (1 << 6));
        bool bg_priority = (sprite.flags & (1 << 7));

        uint8_t y_in_sprite = (ly + 16) - sprite.y;
        if (y_flip) y_in_sprite = sprite_height - 1 - y_in_sprite;

        uint16_t tile_data_addr = 0x8000 + sprite.tile_index * 16;
        uint8_t byte1 = vram[tile_data_addr + y_in_sprite * 2 - 0x8000];
        uint8_t byte2 = vram[tile_data_addr + y_in_sprite * 2 + 1 - 0x8000];

        for (int x = 0; x < 8; ++x) {
            int pixel_x = (sprite.x - 8) + x;
            if (pixel_x < 0 || pixel_x >= 160) continue;

            if (bg_priority && scanline_buffer[pixel_x] != 0) continue;

            uint8_t x_in_tile = x_flip ? x : 7 - x;
            uint8_t color_id = (((byte2 >> x_in_tile) & 1) << 1) | ((byte1 >> x_in_tile) & 1);

            if (color_id == 0) continue;

            framebuffer[ly * 160 + pixel_x] = get_color_from_palette(color_id, palette_reg);
        }
    }
}

// Memory and Register Access
uint8_t PPU::read_vram(uint16_t address) const {
    return vram[address - 0x8000];
}

void PPU::write_vram(uint16_t address, uint8_t value) {
    if (!this->is_vram_accessible()) return;
    vram[address - 0x8000] = value;
}

uint8_t PPU::read_oam(uint16_t addr) const {
    return oam[addr - 0xFE00];
}

void PPU::write_oam(uint16_t addr, uint8_t data) {
    oam[addr - 0xFE00] = data;
}

uint8_t PPU::read_control(uint16_t addr) const {
    switch (addr) {
        case 0xFF40: return lcdc.data;
        case 0xFF41: return stat.read();
        case 0xFF42: return scy;
        case 0xFF43: return scx;
        case 0xFF44: return ly;
        case 0xFF45: return lyc;
        case 0xFF47: return bgp;
        case 0xFF48: return obp0;
        case 0xFF49: return obp1;
        case 0xFF4A: return wy;
        case 0xFF4B: return wx;
        default: return 0xFF;
    }
}

void PPU::write_control(uint16_t addr, uint8_t data) {
    switch (addr) {
        case 0xFF40: lcdc.data = data; break;
        case 0xFF41: stat.write(data); break;
        case 0xFF42: scy = data; break;
        case 0xFF43: scx = data; break;
        case 0xFF44: /* LY is read-only */ break;
        case 0xFF45: lyc = data; break;
        case 0xFF47: bgp = data; break;
        case 0xFF48: obp0 = data; break;
        case 0xFF49: obp1 = data; break;
        case 0xFF4A: wy = data; break;
        case 0xFF4B: wx = data; break;
        default: ;
    }
}
