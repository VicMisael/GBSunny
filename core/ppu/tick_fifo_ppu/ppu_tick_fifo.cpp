

#include "ppu_tick_fifo.h"
#include <algorithm>
#include <ranges>

constexpr int SCANLINE_CYCLES = 456;
constexpr int DRAWING_MAX_CYCLES = 289;
constexpr int FRAME_LINES = 154;

ppu_tick_fifo::ppu_tick_fifo(std::shared_ptr<shared::interrupt> interrupt_controller) :
	interrupt_controller(std::move(interrupt_controller)){
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

	std::ranges::fill(vram, 0x00);
	std::ranges::fill(oam, 0xff);
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

	state.total_dots++;


	switch (stat.ppu_mode) {
	case ppu_types::OAM_SCAN: {
		oam_scan();
		break;
	}
	case ppu_types::DRAWING: {
		if (state.current_x < 160 && state.drawing_cycles++ < DRAWING_MAX_CYCLES) { // Avoid locking on disabled cycles
			render_scanline();
			break;
		}
		set_mode(ppu_types::HBLANK);
		break;
	}
	case ppu_types::HBLANK: {
		if (state.total_dots == SCANLINE_CYCLES) {
			//increment_ly();
			increment_ly();
			if (ly == 144) {
				set_mode(ppu_types::VBLANK);
				
			}
			else {
				set_mode(ppu_types::OAM_SCAN);
			}

			state.hblank_reset();

		};

		break;
	}
	case ppu_types::VBLANK: {
		if (state.total_dots == SCANLINE_CYCLES) {

			increment_ly();
			if (ly == FRAME_LINES) {
				ly = 0;
				set_mode(ppu_types::OAM_SCAN);
			}
			state.vblank_reset();
		}

		break;
	};
	default:;
	};
}
void ppu_tick_fifo::scanline_checks()
{
	if(ly==wy)
	{
		state.window_ly_equals_wy = true;
	}

	check_lyc_coincidence();
}

inline void ppu_tick_fifo::oam_scan()
{
	if(state.oam_cycle==0)
	{
		scanline_checks();
	}
	if (state.oam_cycle != 80) {
		state.oam_cycle++;
	}
	else {
		sprite_buffer.clear();
		fill_oam_buffer();
		set_mode(ppu_types::DRAWING);
		if (ly == wy)
		{
			state.window_ly_equals_wy = true;
		}
	};
}



void ppu_tick_fifo::render_scanline() {
	//if (line_state.first_fetch > 0) {
	//	//delay 12 cycles
	//	line_state.first_fetch--;
	//	if (line_state.first_fetch == 0) {
	//		return; //return and wait for the next cycles from the CPU
	//	}
	//}



	
	render_bg(state.window_triggered);
	


	if (lcdc.bits.OBJ_Enable && oam_render_possible()) {
		render_oam();
	}

	if (!state.oam_fetcher_running && !state.background_fifo.empty()) {
		const ppu_fifo_types::fifo_element bg = state.background_fifo.back();
		state.background_fifo.pop_back();

		auto color = get_color_from_palette(bg.color, bgp);
		if (!state.sprite_fifo.empty()) {
			auto sprite = state.sprite_fifo.back();
			state.sprite_fifo.pop_back();
			uint8_t palette = sprite.palette ? obp1 : obp0;
			const bool sprite_is_opaque = (sprite.color != 0);
			const bool sprite_has_priority = !sprite.bg_priority;
			const bool bg_is_transparent = (bg.color == 0);

			if (sprite_is_opaque && (sprite_has_priority || bg_is_transparent)) {
				color = get_color_from_palette(sprite.color, palette);
			}
		}

		framebuffer[ly * 160 + state.current_x++] = color;
	}



	if (lcdc.bits.window_enable && state.window_ly_equals_wy && !state.window_triggered && state.current_pixel >= wx - 7) {
		state.window_triggered = true;
		state.window_line++;
		state.reset_bg_fifo();
		state.background_fifo_state = ppu_fifo_types::fifo_state::GET_TILE;
	}



}

bool ppu_tick_fifo::oam_render_possible() const{
	
	if (state.oam_fetcher_running) return true;
	for (const auto& element : sprite_buffer)
	{
		if (element.sprite.x <= state.current_x+8)
		{
			return true;
		}
	}
	return false;
}
uint16_t ppu_tick_fifo::extract_tile_map_addr(bool fetching_window) const
{
	if (fetching_window)
	{

		uint16_t tile_map_area = lcdc.bits.window_tile_map_area ? 0x9C00 : 0x9800;
		uint8_t y_in_map = (state.window_line);
		uint8_t tile_row = y_in_map / 8;
		uint8_t x_in_map = (state.current_pixel - (wx - 7));
		uint8_t tile_col = x_in_map / 8;
		return tile_map_area + tile_row * 32 + tile_col;



	}
	uint16_t tile_map_area = lcdc.bits.BG_tile_map ? 0x9C00 : 0x9800;
	uint8_t y_in_map = (scy + ly) & 0xFF;
	uint8_t tile_row = y_in_map / 8;
	uint8_t x_in_map = scx + state.current_pixel;
	uint8_t tile_col = x_in_map / 8;
	return tile_map_area + tile_row * 32 + tile_col;
}

void ppu_tick_fifo::render_bg(bool fetching_window)
{

	switch (state.background_fifo_state) {
	case ppu_fifo_types::fifo_state::GET_TILE:
	{
			if(state.oam_fetcher_running)
			{
				break;
			}
		state.bg_fetcher_running = true;
		if (++state.bg_fetcher_cycle < 2) break;


		state.bg_tile_id = read_vram_internal(extract_tile_map_addr(fetching_window));
		state.background_fifo_state = ppu_fifo_types::fifo_state::GET_TILE_DATA_LOW;
		state.bg_fetcher_cycle = 0;
		break;
	}
	case ppu_fifo_types::fifo_state::GET_TILE_DATA_LOW: {
		if (++state.bg_fetcher_cycle < 2) break;

		uint16_t base = lcdc.bits.BG_window_tiles_adressing ? 0x8000 : 0x8800;

		int tile = lcdc.bits.BG_window_tiles_adressing
			? state.bg_tile_id
			: static_cast<int8_t>(state.bg_tile_id) + 128;

		uint8_t y_in_tile = (fetching_window ? state.window_line : (scy + ly)) % 8;
		uint16_t addr = (base + tile * 16 + y_in_tile * 2);

		state.current_bg_line.lsb = read_vram_internal(addr);

		state.bg_fetcher_cycle = 0;
		state.background_fifo_state = ppu_fifo_types::fifo_state::GET_TILE_DATA_HIGH;
		break;
	}
	case ppu_fifo_types::fifo_state::GET_TILE_DATA_HIGH: {

		if (++state.bg_fetcher_cycle < 2) break;

		const uint16_t base = lcdc.bits.BG_window_tiles_adressing ? 0x8000 : 0x8800;
		const int tile = lcdc.bits.BG_window_tiles_adressing
			? state.bg_tile_id
			: static_cast<int8_t>(state.bg_tile_id) + 128;

		const uint8_t y_in_tile = (fetching_window ? state.window_line : (scy + ly)) % 8;
		const uint16_t addr = ((base + tile * 16 + y_in_tile * 2) + 1);

		state.current_bg_line.msb = read_vram_internal(addr);

		state.bg_fetcher_cycle = 0;
		state.background_fifo_state = ppu_fifo_types::fifo_state::SLEEP;
		break;
	}
	case ppu_fifo_types::fifo_state::SLEEP: {
		state.background_fifo_state = ppu_fifo_types::fifo_state::PUSH;
		[[fallthrough]];
	}
	case ppu_fifo_types::fifo_state::PUSH: {
		if (state.background_fifo.empty()) {
			const auto pixels = state.current_bg_line.decoded_pixels();

			const uint8_t discard = !fetching_window && (state.current_pixel == 0) ? (scx % 8) : 0;


			for (uint8_t i = discard; i < 8; ++i) {
				const uint8_t color = pixels[i];

				ppu_fifo_types::fifo_element element{ .color = color,.bg_priority = color == 0 };

				state.background_fifo.push_front(element);
				state.current_pixel++;
			}

			
			
			state.background_fifo_state = ppu_fifo_types::fifo_state::GET_TILE;
			state.bg_fetcher_running = false;
		}
		state.bg_fetcher_cycle = 0;
		break;
	}
	}
}


uint8_t reverse(uint8_t b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

void ppu_tick_fifo::render_oam() {
	const auto sprite_height = lcdc.bits.OBJ_SIZE ? 16 : 8;
	switch (state.sprite_fifo_state) {
	case ppu_fifo_types::fifo_state::GET_TILE: {
		if (state.bg_fetcher_running)
		{
			break;
		}
		state.oam_fetcher_running = true;
		const auto sprite = sprite_buffer.front();
		
		state.current_sprite = sprite.sprite;
		state.sprite_fifo_state = ppu_fifo_types::fifo_state::GET_TILE_DATA_LOW;
	}
											 break;
	case ppu_fifo_types::fifo_state::GET_TILE_DATA_LOW: {
		const auto& sprite = state.current_sprite;
		uint8_t y_offset = ly + 16 - (sprite.y);
		const bool y_flip = sprite.flags.y_flip;
		if (y_flip) {
			y_offset = sprite_height - 1 - y_offset;
		}
		const auto addr = 0x8000 + sprite.tile_index * 16 + y_offset * 2;
		const auto value = read_vram_internal(addr);
		state.current_oam_line.lsb = sprite.flags.x_flip ? reverse(value) : value;
		state.sprite_fifo_state = ppu_fifo_types::fifo_state::GET_TILE_DATA_HIGH;
	}
													  break;
	case ppu_fifo_types::fifo_state::GET_TILE_DATA_HIGH: {

		const auto& sprite = state.current_sprite;
		const bool y_flip = sprite.flags.y_flip;
		uint8_t y_offset = ly + 16 - (sprite.y);
		if (y_flip) {
			y_offset = sprite_height - 1 - y_offset;
		}
		const uint16_t addr = 1 + 0x8000 + sprite.tile_index * 16 + y_offset * 2;
		const auto value = read_vram_internal(addr);
		state.current_oam_line.msb = sprite.flags.x_flip ? reverse(value) : value;

		state.sprite_fifo_state = ppu_fifo_types::fifo_state::SLEEP;
	}
													   break;
	case ppu_fifo_types::fifo_state::SLEEP:
	{
		state.sprite_fifo_state = ppu_fifo_types::fifo_state::PUSH;
	}
	break;
	case ppu_fifo_types::fifo_state::PUSH: {
		const auto pixel_list = state.current_oam_line.decoded_pixels(state.current_sprite.flags.x_flip);
		const auto sprite = state.current_sprite;
		const bool empty_buffer = state.sprite_fifo.empty();
		for (const auto [x, pixel] : std::views::enumerate(pixel_list)) {
			const int screen_x = sprite.x - 8 + x;
			if (screen_x < 0 || screen_x >= 160) continue;

			ppu_fifo_types::fifo_element element{
					.color = pixel,
					.palette = state.current_sprite.flags.palette_number,
					.bg_priority = state.current_sprite.flags.obj_to_dbg_priority
			};

			if (!empty_buffer)
			{
				const auto current_pixel = state.sprite_fifo[x];
				if (current_pixel.color == 0)
				{
					state.sprite_fifo[x] = element;
				}
			}
			else
			{
				state.sprite_fifo.push_front(element);
			}

		}
		sprite_buffer.pop();
		state.sprite_fifo_state = ppu_fifo_types::fifo_state::GET_TILE;
		state.oam_fetcher_running = false;
		break;

	}
	}
}

void ppu_tick_fifo::step(uint32_t cycles) {
	for (uint32_t i = 0; i < cycles; i++) {
		//Run this tick by tick, step by step, might be slower than every option, Should be more accurate
		tick();
	}
}

uint8_t ppu_tick_fifo::read_vram_internal(uint16_t address) const {
	return vram[address - 0x8000];
}

void ppu_tick_fifo::write_vram_internal(uint16_t address, uint8_t data) {
	vram[address - 0x8000] = data;
}


// Memory and Register Access
uint8_t ppu_tick_fifo::read_vram(uint16_t address) const {
	if (!this->is_vram_accessible()) return 0xff;
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
	if (new_mode == stat.ppu_mode) { return; }
	stat.ppu_mode = new_mode;

	bool interrupt_requested = false;
	switch (new_mode) {
	case ppu_types::HBLANK:
		interrupt_requested = stat.MODE_0_INT_SELECT;  break;
	case ppu_types::VBLANK: {
		interrupt_controller->requested.VBlank = true;
		interrupt_requested = stat.MODE_1_INT_SELECT;
	} break;
	case ppu_types::OAM_SCAN: interrupt_requested = stat.MODE_2_INT_SELECT; break;
	case ppu_types::DRAWING:  break;
	}

	if (interrupt_requested) {
		interrupt_controller->requested.STAT = interrupt_requested;
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
	//Write state
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
	return stat.ppu_mode!= ppu_types::DRAWING ;
}

bool ppu_tick_fifo::is_oam_accessible() const {
	return stat.ppu_mode!= ppu_types::OAM_SCAN && stat.ppu_mode != ppu_types::DRAWING;
}

auto ppu_tick_fifo::get_framebuffer() const -> const std::array<ppu_types::rgba, 160 * 144>&
{
	return framebuffer;
}

void ppu_tick_fifo::increment_ly() {
	ly++;
}

void ppu_tick_fifo::check_lyc_coincidence() {
	stat.LYC_eq_LY = (ly == lyc);
	if (stat.LYC_eq_LY && stat.LYC_INT_SELECT) {
		interrupt_controller->requested.STAT = true;
	}
}



ppu_types::rgba ppu_tick_fifo::get_color_from_palette(uint8_t color_id, uint8_t palette_reg) const {
	int shade_index = (palette_reg >> (color_id * 2)) & 0b11;
	return colors[shade_index];
}

void ppu_tick_fifo::fill_oam_buffer() {
	const auto sprite_height = lcdc.bits.OBJ_SIZE ? 16 : 8;

	uint16_t i = 0;
	for (const auto sprite : oam_sprites) {

		const auto ly_plus_16 = ly + 16;
		if (!sprite_buffer.full() && sprite.x > 0 && ly_plus_16 >= sprite.y && ly_plus_16 < (sprite.y + sprite_height)) {
			ppu_fifo_types::OAM_priority_queue_element element{ .sprite = sprite,.oam_index = i };
			sprite_buffer.push(element);
		}
	}

	const auto comparator = [](const ppu_fifo_types::OAM_priority_queue_element& lhs, const ppu_fifo_types::OAM_priority_queue_element& rhs) {
		if (lhs.sprite.x == rhs.sprite.x) {
			return lhs.oam_index < rhs.oam_index;

		}
		return lhs.sprite.x < rhs.sprite.x;

		};

	sprite_buffer.sort(comparator);



}

