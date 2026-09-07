

#include "ppu_tick_fifo.h"
#include <algorithm>
#include <cstddef>

ppu_tick_fifo::ppu_tick_fifo(std::shared_ptr<shared::interrupt> interrupt_controller) :
	interrupt_controller(std::move(interrupt_controller)) {
	ppu_tick_fifo::reset();
}


void ppu_tick_fifo::reset() {
	const bool was_dma_active = dma_cycles_remaining > 0;
	dma_cycles_remaining = 0;
	if (was_dma_active) {
		notify_dma_changed(false);
	}
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
	stat_interrupt_line = false;
	state.vblank_reset();
	sprite_buffer.clear();
	unlock_vram_access();
	set_mode(ppu_types::OAM_SCAN);

	std::ranges::fill(vram, 0x00);
	std::ranges::fill(oam, 0x00);
}

void ppu_tick_fifo::reset_lcd_state()
{
	ly = 0;
	state.vblank_reset();
	sprite_buffer.clear();
	set_mode(ppu_types::HBLANK);
	check_lyc_coincidence();
}



void inline ppu_tick_fifo::tick()
{
	if (dma_cycles_remaining > 0) {
		dma_cycles_remaining--;
		if (dma_cycles_remaining == 0) {
			notify_dma_changed(false);
		}
	}

 	if (!lcdc.bits.LCD_PPU_enable) {
		reset_lcd_state();
		return;
	}



	state.total_dots++;


	switch (stat.ppu_mode) {
	case ppu_types::OAM_SCAN: {
		oam_scan();
		break;
	}
	case ppu_types::DRAWING: {
		++state.drawing_cycles;
		render_scanline();
		if (state.current_x == static_cast<int>(gb_hardware::display::Width)) {
			set_mode(ppu_types::HBLANK);
		}
		break;
	}
	case ppu_types::HBLANK: {
		if (state.total_dots == gb_hardware::ppu::DotsPerLine) {
			increment_ly();

			if (ly == gb_hardware::ppu::VisibleLines) {
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
		if (state.total_dots == gb_hardware::ppu::DotsPerLine) {

			increment_ly();
			if (ly == gb_hardware::ppu::TotalLines) {
				ly = 0;
				check_lyc_coincidence();
				set_mode(ppu_types::OAM_SCAN);
				state.vblank_reset();
			}
			state.hblank_reset();//Use the same hblank reset
		
		}

		break;
	};
	default:;
	};
}
void ppu_tick_fifo::scanline_checks()
{
	if (ly == wy)
	{
		state.window_ly_equals_wy = true;
	}
}

inline void ppu_tick_fifo::oam_scan()
{
	if (state.oam_cycle == 0)
	{
		scanline_checks();
		check_lyc_coincidence();
	}
	state.oam_cycle++;
	if (state.oam_cycle >= static_cast<int>(gb_hardware::ppu::OamScanDots)) {
		sprite_buffer.clear();
		fill_oam_buffer();
		state.discard_pixels = scx & 7;
		set_mode(ppu_types::DRAWING);
		if (ly == wy)
		{
			state.window_ly_equals_wy = true;
		}
	}
}



void ppu_tick_fifo::render_scanline() {
	if (state.startup_dots > 0) {
		--state.startup_dots;
		render_bg(state.window_triggered);
		return;
	}

	if (state.discard_pixels > 0) {
		if (!state.background_fifo.empty()) {
			state.background_fifo.pop_front();
			--state.discard_pixels;
		}
		render_bg(state.window_triggered);
		return;
	}

	if (state.oam_fetcher_running) {
		render_oam();
		return;
	}

	const int window_x = static_cast<int>(wx) - 7;
	if (lcdc.bits.BG_window_enable && lcdc.bits.window_enable && state.window_ly_equals_wy
		&& !state.window_triggered && state.current_x >= window_x) {
		state.window_triggered = true;
		state.window_line++;
		state.window_start_x = window_x;
		state.current_pixel = 0;
		state.discard_pixels = std::max(0, -window_x);
		state.last_sprite_tile = -1;
		state.reset_bg_fifo();
		state.bg_fetcher_cycle = 0;
		state.background_fifo_state = ppu_fifo_types::fifo_state::GET_TILE;
		render_bg(true);
		return;
	}

	if (state.background_fifo.empty()) {
		render_bg(state.window_triggered);
		return;
	}

	if (lcdc.bits.OBJ_Enable && oam_render_possible()) {
		state.current_sprite = sprite_buffer.front().sprite;
		sprite_buffer.pop();
		const int tile_pixel = state.window_triggered
			? state.current_x - state.window_start_x : state.current_x + (scx & 7);
		const int tile = tile_pixel / 8;
		state.sprite_wait_dots = tile != state.last_sprite_tile ? std::max(0, 5 - (tile_pixel & 7)) : 0;
		state.last_sprite_tile = tile;
		if (state.current_sprite.x == 0) {
			state.sprite_wait_dots = 5;
		}
		state.sprite_fetch_cycle = 0;
		state.sprite_fifo_state = ppu_fifo_types::fifo_state::GET_TILE;
		state.oam_fetcher_running = true;
		render_oam();
		return;
	}

	const auto bg = state.background_fifo.front();
	state.background_fifo.pop_front();
	const uint8_t bg_color = lcdc.bits.BG_window_enable ? bg.color : 0;
	auto color = lcdc.bits.BG_window_enable ? get_color_from_palette(bg_color, bgp) : colors[0];
	if (!state.sprite_fifo.empty()) {
		const auto sprite = state.sprite_fifo.front();
		state.sprite_fifo.pop_front();
		if (lcdc.bits.OBJ_Enable && sprite.color != 0 && (!sprite.bg_priority || bg_color == 0)) {
			color = get_color_from_palette(sprite.color, sprite.palette ? obp1 : obp0);
		}
	}
	framebuffer[ly * gb_hardware::display::Width + state.current_x++] = color;
	render_bg(state.window_triggered);
}

bool ppu_tick_fifo::oam_render_possible() const {
	return !sprite_buffer.empty() && sprite_buffer.front().sprite.x <= state.current_x + 8;
}
uint16_t ppu_tick_fifo::extract_tile_map_addr(bool fetching_window) const
{
	if (fetching_window)
	{

		uint16_t tile_map_area = lcdc.bits.window_tile_map_area ? 0x9C00 : 0x9800;
		uint8_t y_in_map = (state.window_line);
		uint8_t tile_row = y_in_map / 8;
		uint8_t x_in_map = static_cast<uint8_t>(state.current_pixel);
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
		state.background_fifo_state = ppu_fifo_types::fifo_state::PUSH;
		[[fallthrough]];
	}
	case ppu_fifo_types::fifo_state::PUSH: {
		if (state.background_fifo.empty()) {
			const auto pixels = state.current_bg_line.decoded_pixels();
			for (const uint8_t color : pixels) {

				ppu_fifo_types::fifo_element element{ .color = color,.bg_priority = color == 0 };

				state.background_fifo.push_back(element);
				state.current_pixel++;
			}



			state.bg_fetcher_cycle = 0;
			state.background_fifo_state = ppu_fifo_types::fifo_state::SLEEP;
			state.bg_fetcher_running = false;
			break;
		}
		break;
	}
	case ppu_fifo_types::fifo_state::SLEEP: {
		if (++state.bg_fetcher_cycle < 2) break;
		state.bg_fetcher_cycle = 0;
		state.background_fifo_state = ppu_fifo_types::fifo_state::GET_TILE;
		break;
	}
	}
}

void ppu_tick_fifo::render_oam() {
	if (state.sprite_wait_dots > 0) {
		--state.sprite_wait_dots;
		return;
	}
	if (++state.sprite_fetch_cycle < 2) return;
	state.sprite_fetch_cycle = 0;

	const auto& sprite = state.current_sprite;
	const int sprite_height = lcdc.bits.OBJ_SIZE ? 16 : 8;
	int y_offset = static_cast<int>(ly) + 16 - sprite.y;
	if (sprite.flags.y_flip) {
		y_offset = sprite_height - 1 - y_offset;
	}
	const uint8_t tile_index = lcdc.bits.OBJ_SIZE ? (sprite.tile_index & 0xFE) : sprite.tile_index;
	const uint16_t address = 0x8000 + tile_index * 16 + y_offset * 2;

	switch (state.sprite_fifo_state) {
	case ppu_fifo_types::fifo_state::GET_TILE:
		state.sprite_fifo_state = ppu_fifo_types::fifo_state::GET_TILE_DATA_LOW;
		break;
	case ppu_fifo_types::fifo_state::GET_TILE_DATA_LOW:
		state.current_oam_line.lsb = read_vram_internal(address);
		state.sprite_fifo_state = ppu_fifo_types::fifo_state::GET_TILE_DATA_HIGH;
		break;
	case ppu_fifo_types::fifo_state::GET_TILE_DATA_HIGH: {
		state.current_oam_line.msb = read_vram_internal(address + 1);
		const auto pixels = state.current_oam_line.decoded_pixels(sprite.flags.x_flip);
		while (!state.sprite_fifo.full()) {
			state.sprite_fifo.push_back({});
		}
		for (int pixel = 0; pixel < 8; ++pixel) {
			const int offset = static_cast<int>(sprite.x) - 8 + pixel - state.current_x;
			if (offset < 0 || offset >= 8 || pixels[pixel] == 0) continue;
			auto& queued = state.sprite_fifo[static_cast<std::size_t>(offset)];
			if (queued.color != 0) continue;
			queued = {
				.color = pixels[pixel],
				.palette = sprite.flags.palette_number,
				.bg_priority = sprite.flags.obj_to_dbg_priority
			};
		}
		state.sprite_fifo_state = ppu_fifo_types::fifo_state::GET_TILE;
		state.oam_fetcher_running = false;
		break;
	}
	default:
		break;
	}
}

void ppu_tick_fifo::step(uint32_t cycles) {
	for (uint32_t i = 0; i < cycles; i++) {
		//Run this tick by tick, step by step, might be slower than every option, Should be more accurate
		tick();
	}
}

uint8_t ppu_tick_fifo::read_vram_internal(const uint16_t address) const {
	return vram[address - 0x8000];
}

void ppu_tick_fifo::write_vram_internal(uint16_t address, uint8_t data) {
	vram[address - 0x8000] = data;
}


// Memory and Register Access
uint8_t ppu_tick_fifo::read_vram(uint16_t address) const {
	if (!this->vram_accessible) return 0xff;
	return vram[address - 0x8000];
}

void ppu_tick_fifo::write_vram(uint16_t address, uint8_t value) {
	if (!this->vram_accessible) return;
	vram[address - 0x8000] = value;
}

void ppu_tick_fifo::set_mode(ppu_types::ppu_mode new_mode) {
	if (new_mode == stat.ppu_mode) { return; }
	const bool leaving_drawing = stat.ppu_mode == ppu_types::DRAWING;
	stat.ppu_mode = new_mode;
	if (leaving_drawing && new_mode != ppu_types::DRAWING) {
		unlock_vram_access();
	}

	switch (new_mode) {
	case ppu_types::VBLANK: {
		interrupt_controller->requested.VBlank = true;
		break;
	}
	case ppu_types::DRAWING:
		{
			lock_vram_access();
			break;
		}
	break;
	default:
		break;
	}
	update_stat_interrupt_line();
}

void ppu_tick_fifo::lock_vram_access()
{
	vram_accessible = false;
	notify_vram_access_changed(false);
}

void ppu_tick_fifo::unlock_vram_access()
{
	vram_accessible = true;
	notify_vram_access_changed(true);
}


bool ppu_tick_fifo::stat_interrupt_signal() const {
	return (stat.LYC_eq_LY && stat.LYC_INT_SELECT)
		|| (stat.ppu_mode == ppu_types::HBLANK && stat.MODE_0_INT_SELECT)
		|| (stat.ppu_mode == ppu_types::VBLANK && stat.MODE_1_INT_SELECT)
		|| (stat.ppu_mode == ppu_types::OAM_SCAN && stat.MODE_2_INT_SELECT);
}

void ppu_tick_fifo::update_stat_interrupt_line() {
	const bool signal = stat_interrupt_signal();
	if (signal && !stat_interrupt_line) {
		interrupt_controller->requested.STAT = true;
	}
	stat_interrupt_line = signal;
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
	case 0xFF40: {
		const bool was_enabled = lcdc.bits.LCD_PPU_enable;
		lcdc.data = data;
		if (was_enabled && !lcdc.bits.LCD_PPU_enable) {
			reset_lcd_state();
		}
	} break;
	case 0xFF41:
		stat.write(data);
		update_stat_interrupt_line();
		break;
	case 0xFF42: scy = data; break;
	case 0xFF43: scx = data; break;
	case 0xFF44: /* LY is read-only */ break;
	case 0xFF45:
	{
		lyc = data;
		check_lyc_coincidence();
	} break;
	case 0xFF46: start_dma_transfer(); break;
	case 0xFF47: bgp = data; break;
	case 0xFF48: obp0 = data; break;
	case 0xFF49: obp1 = data; break;
	case 0xFF4A: wy = data; break;
	case 0xFF4B: wx = data; break;
	default:;
	}
}

void ppu_tick_fifo::start_dma_transfer() {
	const bool was_dma_active = dma_cycles_remaining > 0;
	dma_cycles_remaining = gb_hardware::ppu::DmaCycles;
	if (!was_dma_active) {
		notify_dma_changed(true);
	}
}

bool ppu_tick_fifo::is_dma_active() const {
	return dma_cycles_remaining > 0;
}
bool ppu_tick_fifo::is_oam_accessible() const {
	return dma_cycles_remaining == 0 && stat.ppu_mode != ppu_types::OAM_SCAN && stat.ppu_mode != ppu_types::DRAWING;
}

bool ppu_tick_fifo::is_vram_accessible() const
{
	return this->vram_accessible;
}

auto ppu_tick_fifo::get_framebuffer() const -> const std::array<ppu_types::rgba, gb_hardware::display::PixelCount>&
{
	return framebuffer;
}

void ppu_tick_fifo::increment_ly() {
	ly++;
	check_lyc_coincidence();
}

void ppu_tick_fifo::check_lyc_coincidence() {
	stat.LYC_eq_LY = (ly == lyc);
	update_stat_interrupt_line();
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
		if (!sprite_buffer.full() && ly_plus_16 >= sprite.y && ly_plus_16 < (sprite.y + sprite_height)) {
			ppu_fifo_types::OAM_priority_queue_element element{ .sprite = sprite,.oam_index = i };
			sprite_buffer.push(element);
		}
		++i;
	}

	const auto comparator = [](const ppu_fifo_types::OAM_priority_queue_element& lhs, const ppu_fifo_types::OAM_priority_queue_element& rhs) {
		if (lhs.sprite.x == rhs.sprite.x) {
			return lhs.oam_index < rhs.oam_index;

		}
		return lhs.sprite.x < rhs.sprite.x;

		};

	sprite_buffer.sort(comparator);



}
