/*
 * =====================================================================================
 *
 * Filename:  ppu.cpp
 *
 * =====================================================================================
 */

#include "ppu_scanline.h"
#include <algorithm>
#include <vector>

// PPU timings in T-cycles
constexpr int OAM_SCAN_CYCLES = 80;
constexpr int DRAWING_CYCLES = 172;
constexpr int HBLANK_CYCLES = 204;
constexpr int SCANLINE_CYCLES = 456;
constexpr int VBLANK_LINES = 10;
constexpr int FRAME_LINES = 154;

PPU_scanline::PPU_scanline(std::shared_ptr<shared::interrupt> interrupt_controller)
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

void PPU_scanline::reset() {
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

void PPU_scanline::step(uint32_t cycles_to_run) {
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
                sprite_buffer_index=0;
                fill_oam_buffer();
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
                    interrupt_controller->requested.VBlank = true;
                    set_mode(ppu_types::VBLANK);
                    // Correctly set the VBlank interrupt requested

                    
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

void PPU_scanline::increment_ly() {
    ly++;
    check_lyc_coincidence();
}

void PPU_scanline::check_lyc_coincidence() {
    stat.LYC_eq_LY = (ly == lyc);
    if (stat.LYC_eq_LY && stat.LYC_INT_SELECT) {
        interrupt_controller->requested.STAT = true;
    }
}

void PPU_scanline::set_mode(ppu_types::ppu_mode new_mode) {
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
        interrupt_controller->requested.STAT = true;
    }
}

void PPU_scanline::start_dma_transfer() {
    dma_cycles_remaining = 640;
}

bool PPU_scanline::is_dma_active() const {
    return dma_cycles_remaining > 0;
}

bool PPU_scanline::is_vram_accessible() const {
    return current_mode != ppu_types::DRAWING && dma_cycles_remaining == 0;
}

bool PPU_scanline::is_oam_accessible() const {
    return current_mode != ppu_types::OAM_SCAN && current_mode != ppu_types::DRAWING;
}

const std::array<ppu_types::rgba, 160 * 144>& PPU_scanline::get_framebuffer() const {
    return framebuffer;
}

ppu_types::rgba PPU_scanline::get_color_from_palette(uint8_t color_id, uint8_t palette_reg) const {
    int shade_index = (palette_reg >> (color_id * 2)) & 0b11;
    return colors[shade_index];
}

void PPU_scanline::render_scanline() {
    // This logic remains the same
    if (lcdc.bits.BG_window_enable) {
        render_background();
        render_window();
    }
    if (lcdc.bits.OBJ_Enable) {
        render_sprites();
    }

    for (int i = 0; i < 160; i++) {
        const auto [ color_id, bgp ] = scanline_buffer[i];
        framebuffer[ly * 160 + i] = get_color_from_palette(color_id, bgp);
    }
}

void PPU_scanline::render_background() {
    uint16_t base = (lcdc.bits.BG_window_tiles_adressing ? 0x8000 : 0x8800);
    uint16_t tile_map_area = (lcdc.bits.BG_tile_map ? 0x9C00 : 0x9800);
    uint8_t y_in_map = scy + ly;
    uint8_t tile_row = y_in_map / 8;

    for (int pixel = 0; pixel < 160; ++pixel) {
        uint8_t x_in_map = scx + pixel;
        uint8_t tile_col = x_in_map / 8;

        uint16_t tile_map_addr = tile_map_area + tile_row * 32 + tile_col;
        uint8_t tile_id = read_vram(tile_map_addr);

        uint16_t tile_data_addr;
        if (lcdc.bits.BG_window_tiles_adressing) {
            tile_data_addr =  tile_id;
        } else {
            tile_data_addr =  (static_cast<int8_t>(tile_id) + 128);
        }

        uint8_t y_in_tile = y_in_map % 8;
        uint8_t byte1 = read_vram(base + tile_data_addr*16 + y_in_tile * 2 );
        uint8_t byte2 = read_vram(base + tile_data_addr*16 + y_in_tile * 2 + 1);
        uint8_t x_in_tile = 7 - (x_in_map % 8);
        uint8_t color_id = (((byte2 >> x_in_tile) & 1) << 1) | ((byte1 >> x_in_tile) & 1);

        scanline_buffer[pixel] = { color_id,bgp };
        //framebuffer[ly * 160 + pixel] = get_color_from_palette(color_id, bgp);
    }
}

void PPU_scanline::render_window() {
    if (!lcdc.bits.window_enable || ly < wy) return;

    uint16_t tile_data_area = lcdc.bits.BG_window_tiles_adressing ? 0x8000 : 0x8800;
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
        if (lcdc.bits.BG_window_tiles_adressing) {
            tile_data_addr = tile_data_area + tile_id * 16;
        } else {
            tile_data_addr = tile_data_area + (static_cast<int8_t>(tile_id) + 128) * 16;
        }

        uint8_t y_in_tile = y_in_map % 8;
        uint8_t byte1 = read_vram(tile_data_addr + y_in_tile * 2);
        uint8_t byte2 = read_vram(tile_data_addr + y_in_tile * 2 + 1);
        uint8_t x_in_tile = 7 - (x_in_map % 8);
        uint8_t color_id = (((byte2 >> x_in_tile) & 1) << 1) | ((byte1 >> x_in_tile) & 1);

        scanline_buffer[pixel] = { .color_id = color_id,.bgp = bgp };
        //framebuffer[ly * 160 + pixel] = get_color_from_palette(color_id, bgp);
    }
    window_line_counter++;
}


void PPU_scanline::fill_oam_buffer()
{
    const auto spriteheight = lcdc.bits.OBJ_SIZE ? 16 : 8;
    for (const auto sprite : oam_sprites) {

        const auto ly_plus_16 = ly + 16;
        if (sprite_buffer_index < 10 && sprite.x > 0 && ly_plus_16 >= sprite.y && ly_plus_16 < (sprite.y + spriteheight)) {
           sprite_buffer[sprite_buffer_index++]=sprite;
        }
    }
}


void PPU_scanline::render_sprites() {
    uint8_t sprite_height = lcdc.bits.OBJ_SIZE ? 16 : 8;

    const auto comparator = [](const ppu_types::OAM_Sprite& a, const ppu_types::OAM_Sprite& b) {
        if (a.x != b.x) return a.x >= b.x;
        return &a < &b; // Stable sort for sprites with same X
    };

    std::sort(sprite_buffer.begin(), sprite_buffer.begin()+sprite_buffer_index, comparator);


    for (int i = 0; i < sprite_buffer_index;i++) {
        const auto& sprite = sprite_buffer[i];
        uint8_t palette_reg = (sprite.flags.palette_number) ? obp1 : obp0;
        bool x_flip = (sprite.flags.x_flip);
        bool y_flip = (sprite.flags.y_flip);
        bool bg_priority = (sprite.flags.obj_to_dbg_priority);

        uint8_t y_in_sprite = (ly + 16) - sprite.y;
        if (y_flip) y_in_sprite = sprite_height - 1 - y_in_sprite;

        uint16_t tile_data_addr = 0x8000 + sprite.tile_index * 16;
        uint8_t byte1 = read_vram(tile_data_addr + y_in_sprite * 2);
        uint8_t byte2 = read_vram(tile_data_addr + y_in_sprite * 2 + 1);

        for (int x = 0; x < 8; ++x) {
            int pixel_x = (sprite.x - 8) + x;
            if (pixel_x < 0 || pixel_x >= 160) continue;

            if (bg_priority && scanline_buffer[pixel_x].color_id != 0) continue;

            uint8_t x_in_tile = x_flip ? x : 7 - x;
            uint8_t color_id = (((byte2 >> x_in_tile) & 1) << 1) | ((byte1 >> x_in_tile) & 1);

            if (color_id == 0) continue;
            scanline_buffer[pixel_x] = { color_id,palette_reg };


        }
    }
}

// Memory and Register Access
uint8_t PPU_scanline::read_vram(uint16_t address) const {
    //if(!this->is_vram_accessible()) return 0xff;
    return vram[address - 0x8000];
}

void PPU_scanline::write_vram(uint16_t address, uint8_t value) {
    if (!this->is_vram_accessible()) return;
    vram[address - 0x8000] = value;
}

uint8_t PPU_scanline::read_oam(uint16_t addr) const {
    return oam[addr - 0xFE00];
}

void PPU_scanline::write_oam(uint16_t addr, uint8_t data) {
    oam[addr - 0xFE00] = data;
}

uint8_t PPU_scanline::read_control(uint16_t addr) const {
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

void PPU_scanline::write_control(uint16_t addr, uint8_t data) {
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
