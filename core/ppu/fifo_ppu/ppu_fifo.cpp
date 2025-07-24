#include "ppu_fifo.h"
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
	lcdc(0) {

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

	std::fill(std::begin(vram), std::end(vram), 0x00);
	std::fill(std::begin(oam), std::end(oam), 0xff);
}

void PPU::step(uint32_t cycles_to_run) {
	//Cycles to Run in T-Cycles 
	//Remember on M = 4 TCycles
	//1 dot in DMG = 1 T Cyclkes
	if (dma_cycles_remaining > 0) {
		dma_cycles_remaining -= (cycles_to_run);
		if (dma_cycles_remaining < 0) dma_cycles_remaining = 0;
		return;
	}

	if (!lcdc.bits.LCD_PPU_enable) {
		cycle_counter = 0;
		ly = 0;
		set_mode(ppu_types::HBLANK);
		return;
	}


	switch (current_mode) {
	case ppu_types::OAM_SCAN: {
		if ((state.oam_dots + cycles_to_run) < OAM_SCAN_CYCLES) {
			state.oam_dots += cycles_to_run;


			break;
		}
		else {
			fill_oam_buffer();
			set_mode(ppu_types::DRAWING);
			state.oam_dots += cycles_to_run;
			if (state.oam_dots == OAM_SCAN_CYCLES) {
				break;
				//Nocycles to leak
			}
			cycles_to_run = state.oam_dots % OAM_SCAN_CYCLES; //Leaks the rest of the cycles to the next step
			state.oam_dots = OAM_SCAN_CYCLES;//Just set to 80

			// Oam Scan wraps
		   //leak the scans to next mode 
		}
	}
	case ppu_types::DRAWING: {
		
		render_scanline(cycles_to_run);

		break;
	};
	case ppu_types::HBLANK: {


		increment_ly();

		break;
	};
	case ppu_types::VBLANK: {
		break;
	}

	};
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
	case ppu_types::HBLANK: 
		interrupt_requested = stat.MODE_0_INT_SELECT;  break;
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

void PPU::fill_oam_buffer()
{
	for (const auto& sprite : oam_sprites) {
		const auto spriteheight = (lcdc.bits.OBJ_SIZE + 1) * 8;
		const auto ly_plus_16 = ly + 16;
		if (check_oam_fill_conditions(sprite, ly_plus_16, spriteheight)) {
			state.sprite_buffer[state.sprite_buffer_count++] = sprite;
		}
	}
}

bool PPU::check_oam_fill_conditions(const ppu_types::OAM_Sprite& sprite, const int ly_plus_16, const int spriteheight)
{
	return state.sprite_buffer_count < 10 && sprite.x>0 && ly_plus_16 >= sprite.y && ly_plus_16 < (sprite.y + spriteheight);
}

void PPU::flush_to_buffer() {

}

void PPU::render_scanline(int cycles) {
	// This logic remains the same
	if (lcdc.bits.BG_window_enable) {
		render_background(cycles);
		//render_window(cycles);    
	}
	if (lcdc.bits.OBJ_Enable) {
		//render_sprites(cycles);
	}

	flush_to_buffer();
}

void PPU::render_background(int cycles) {
	//The cycle budget for this run

	//BG 32x32 Tiles

	if (lcdc.bits.BG_window_enable && state.background_fifo.empty()) {
		const auto address_mode = lcdc.bits.BG_window_tiles_adressing ? 0x8000 : 0x8800;
		const auto tile_map_area = lcdc.bits.BG_tile_map ? 0x9C00 : 0x9800;

		//$9C00-$9FFF. Otherwise, it uses $9800-$9BFF
		uint8_t y_in_map = scy + ly;
		uint8_t tile_row = y_in_map / 8;
		uint8_t x_in_map = scx + state.current_drawing_pixel;
	    uint8_t tile_col = x_in_map / 8;
		//Step 1:Tile Lookup
		uint16_t tile_map_addr = tile_map_area + tile_row * 32 + tile_col;
        uint8_t tile_id = vram[tile_map_addr - 0x8000];

        uint16_t tile_data_addr;
        if (lcdc.bits.BG_window_tiles_adressing) {
            tile_data_addr = address_mode + tile_id * 16;
        }
        else {
            tile_data_addr = address_mode + (static_cast<int8_t>(tile_id) + 128) * 16;
        }


	}



}

void PPU::render_window(int cycles) {


}

void PPU::render_sprites(int cycles) {
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
	case 0xFF46:start_dma_transfer(); break;
	case 0xFF47: bgp = data; break;
	case 0xFF48: obp0 = data; break;
	case 0xFF49: obp1 = data; break;
	case 0xFF4A: wy = data; break;
	case 0xFF4B: wx = data; break;
	default:;
	}
}
