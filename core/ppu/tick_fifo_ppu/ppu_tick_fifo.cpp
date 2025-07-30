

#include "ppu_tick_fifo.h"
constexpr int SCANLINE_CYCLES = 456;
constexpr int DRAWING_MAX_CYCLES = 289;
constexpr int FRAME_LINES = 154;

ppu_tick_fifo::ppu_tick_fifo(std::shared_ptr<shared::interrupt> interrupt_controller) :
	interrupt_controller(std::move(interrupt_controller)),
	lcdc(0) {
	ppu_tick_fifo::reset();
}


void ppu_tick_fifo::reset() {
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
	set_mode(ppu_types::OAM_SCAN);

	std::fill(std::begin(vram), std::end(vram), 0x00);
	std::fill(std::begin(oam), std::end(oam), 0xff);
}

void inline ppu_tick_fifo::tick()
{
	if (!lcdc.bits.LCD_PPU_enable) {
		ly = 0;
		set_mode(ppu_types::HBLANK);
		return;
	}

	if (dma_cycles_remaining > 0) {
		dma_cycles_remaining--;
		return;
	}

	line_state.total_dots++;
	switch (stat.ppu_mode) {
	case ppu_types::OAM_SCAN: {
		if (line_state.oam_cycle != 80) {
			line_state.oam_cycle++;
		}
		else {
			fill_oam_buffer();
			set_mode(ppu_types::DRAWING);
		};
		break;
	}
	case ppu_types::DRAWING: {
		if (line_state.current_x < 160 && line_state.drawing_cycles++<DRAWING_MAX_CYCLES) { // Avoid locking on disabled cycles
			render_scanline();
			break;
		}
		else {
			set_mode(ppu_types::HBLANK);
		}
		break;
	}
	case ppu_types::HBLANK: {
		if (line_state.total_dots == SCANLINE_CYCLES) {
			increment_ly();
			if (ly == 144) {
				set_mode(ppu_types::VBLANK);
				interrupt_controller->flag.VBlank = true;
			}
			else {
				set_mode(ppu_types::OAM_SCAN);
			}

			line_state.reset();

		};

		break;
	}
	case ppu_types::VBLANK: {
		if (line_state.total_dots == SCANLINE_CYCLES) {

			increment_ly();
			if (ly == FRAME_LINES) {
				ly = 0;
				set_mode(ppu_types::OAM_SCAN);
			}
			line_state.reset();
		}

		break;
	};
	default:;
	};
}

void ppu_tick_fifo::render_scanline() {
	if (line_state.first_fetch > 0) {
		//delay 12 cycles
		line_state.first_fetch--;
		if (line_state.first_fetch == 0) {
			return; //return and wait for the next cycles from the CPU
		}
	}


	bool is_window = false;
	if (lcdc.bits.window_enable && wy <= ly) {
		if (line_state.window_triggered == false && line_state.current_pixel >= wx - 7) {
			line_state.window_triggered = true;
			line_state.window_line++; // Start from 0 on first line, increment every scanline after trigger
			line_state.reset_bg_fifo();
		}
		is_window = line_state.window_triggered;
	}
	//BG 32x32 Tiles

	if (lcdc.bits.BG_window_enable && line_state.background_fifo.size() <= 8) {
		switch (line_state.background_fifo_state) {
		case ppu_fifo_types::bg_fifo_state::GET_TILE:
		{
			if (++line_state.bg_fetcher_cycle < 2) break;
			uint16_t tile_map_area = lcdc.bits.BG_tile_map ? 0x9C00 : 0x9800;
			uint8_t y_in_map = (scy + ly)&0xFF;
			uint8_t tile_row = y_in_map / 8;
			uint8_t x_in_map = scx + line_state.current_pixel;
			uint8_t tile_col = x_in_map / 8;
			uint16_t tile_map_addr = tile_map_area + tile_row * 32 + tile_col;

			line_state.bg_tile_id = read_vram(tile_map_addr);
			line_state.background_fifo_state = ppu_fifo_types::bg_fifo_state::GET_TILE_DATA_LOW;
			line_state.bg_fetcher_cycle = 0;
			break;
		};
		case ppu_fifo_types::bg_fifo_state::GET_TILE_DATA_LOW: {
			if (++line_state.bg_fetcher_cycle < 2) break;

			uint16_t base = lcdc.bits.BG_window_tiles_adressing ? 0x8000 : 0x8800;

			int tile = lcdc.bits.BG_window_tiles_adressing
				? line_state.bg_tile_id
				: static_cast<int8_t>(line_state.bg_tile_id) + 128;

			uint8_t y_in_tile = (scy + ly) % 8;
			uint16_t addr = (base + tile * 16 + y_in_tile * 2);

			line_state.current_bg_line.lsb = read_vram(addr);

			line_state.bg_fetcher_cycle = 0;
			line_state.background_fifo_state = ppu_fifo_types::bg_fifo_state::GET_TILE_DATA_HIGH;
			break;
		};
		case ppu_fifo_types::bg_fifo_state::GET_TILE_DATA_HIGH: {

			if (++line_state.bg_fetcher_cycle < 2) break;

			uint16_t base = lcdc.bits.BG_window_tiles_adressing ? 0x8000 : 0x8800;
			int tile = lcdc.bits.BG_window_tiles_adressing
				? line_state.bg_tile_id
				: static_cast<int8_t>(line_state.bg_tile_id) + 128;

			uint8_t y_in_tile = (scy + ly) % 8;
			uint16_t addr = ((base + tile * 16 + y_in_tile * 2)+ 1);

			line_state.current_bg_line.msb = read_vram(addr);

			line_state.bg_fetcher_cycle = 0;
			line_state.background_fifo_state = ppu_fifo_types::bg_fifo_state::SLEEP;
			break;
		}
		case ppu_fifo_types::bg_fifo_state::SLEEP: {
			line_state.background_fifo_state = ppu_fifo_types::bg_fifo_state::PUSH;
			[[fallthrough]];
		}
		case ppu_fifo_types::bg_fifo_state::PUSH: {

			if (line_state.background_fifo.empty()) {
				const auto pixels = line_state.current_bg_line.decoded_pixels();

				if (line_state.current_pixel == 0) {
					int discard = scx % 8;
					line_state.discard_delay = discard;
				}


				int discard = (line_state.current_pixel == 0) ? (scx % 8) : 0;
				
				for (int i = discard; i < 8; ++i) {
					uint8_t color = pixels[i];

					ppu_fifo_types::fifo_element element{ .color = color,.bg_priority = color == 0 };

					line_state.background_fifo.push(element);
				}

				line_state.current_pixel += 8;
				line_state.bg_fetcher_cycle = 0;
				line_state.background_fifo_state = ppu_fifo_types::bg_fifo_state::GET_TILE;
				break;
			}
		};
		};
	}

	if (!line_state.background_fifo.empty()) {
		const ppu_fifo_types::fifo_element pixel = line_state.background_fifo.front();
		line_state.background_fifo.pop();
	

		// Framebuffer write (using state.current_pixel or pixel_x++)
		framebuffer[ly*160 + line_state.current_x++] = get_color_from_palette(pixel.color, bgp);
	}


}

void ppu_tick_fifo::step(uint32_t cycles) {
	for (int i = 0; i < cycles; i++) {
		//Run this tick by tick, step by step, might be slower than every option, Should be more accurate
		tick();
	}
}


// Memory and Register Access
uint8_t ppu_tick_fifo::read_vram(uint16_t address) const {
	
	return vram[address - 0x8000];
}

void ppu_tick_fifo::write_vram(uint16_t address, uint8_t value) {
	if (!this->is_vram_accessible()) return;
	vram[address - 0x8000] = value;
}

uint8_t ppu_tick_fifo::read_oam(uint16_t addr) const {
	return oam[addr - 0xFE00];
}

void ppu_tick_fifo::write_oam(uint16_t addr, uint8_t data) {
	oam[addr - 0xFE00] = data;
}

void ppu_tick_fifo::set_mode(ppu_types::ppu_mode new_mode) {
	line_state.current_mode = new_mode;
	stat.ppu_mode = new_mode;

	bool interrupt_requested = false;
	switch (new_mode) {
	case ppu_types::HBLANK:
		interrupt_requested = stat.MODE_0_INT_SELECT;  break;
	case ppu_types::VBLANK: {
		interrupt_requested = stat.MODE_1_INT_SELECT;
	} break;
	case ppu_types::OAM_SCAN: interrupt_requested = stat.MODE_2_INT_SELECT; break;
	case ppu_types::DRAWING:  break;
	}

	if (interrupt_requested) {
		interrupt_controller->flag.LCD = true;
	}
}

uint8_t ppu_tick_fifo::read_control(uint16_t addr) const {
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

void ppu_tick_fifo::write_control(uint16_t addr, uint8_t data) {
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
	default:;
	}
}

void ppu_tick_fifo::start_dma_transfer() {
	dma_cycles_remaining = 640;
}

bool ppu_tick_fifo::is_dma_active() const {
	return dma_cycles_remaining > 0;
}

bool ppu_tick_fifo::is_vram_accessible() const {
	return line_state.current_mode != ppu_types::DRAWING && dma_cycles_remaining == 0;
}

bool ppu_tick_fifo::is_oam_accessible() const {
	return line_state.current_mode != ppu_types::OAM_SCAN && line_state.current_mode != ppu_types::DRAWING;
}

const std::array<ppu_types::rgba, 160 * 144>& ppu_tick_fifo::get_framebuffer() const {
	return framebuffer;
}

void ppu_tick_fifo::increment_ly() {
	ly++;
	if (ly >= 0x3E) {
		check_lyc_coincidence();
	}
	else
		check_lyc_coincidence();
}

void ppu_tick_fifo::check_lyc_coincidence() {
	stat.LYC_eq_LY = (ly == lyc);
	if (stat.LYC_eq_LY && stat.LYC_INT_SELECT) {
		interrupt_controller->flag.LCD = true;
	}
}



ppu_types::rgba ppu_tick_fifo::get_color_from_palette(uint8_t color_id, uint8_t palette_reg) const {
	int shade_index = (palette_reg >> (color_id * 2)) & 0b11;
	return colors[shade_index];
}

void ppu_tick_fifo::fill_oam_buffer() {
	const auto spriteheight = lcdc.bits.OBJ_SIZE ? 16 : 8;

	for (const auto sprite : oam_sprites) {

		const auto ly_plus_16 = ly + 16;
		if (sprite_buffer_index < 10 && sprite.x > 0 && ly_plus_16 >= sprite.y && ly_plus_16 < (sprite.y + spriteheight)) {
			sprite_buffer[sprite_buffer_index++] = sprite;
		}
	}
}

