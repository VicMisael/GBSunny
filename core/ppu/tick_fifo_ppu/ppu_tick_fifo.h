#ifndef PPU_TICK_FIFO_H
#define PPU_TICK_FIFO_H
#include "../ppu_base.h"
#include "shared/interrupt.h"
#include "ppu_fifo_types.h"
#include <queue>
#include "ppu_fifo_types.h"



class ppu_tick_fifo :public PPU_Base {
public:
	//Constructor takes a shared_ptr to the interrupt controller
	explicit ppu_tick_fifo(std::shared_ptr<shared::interrupt> interrupt_controller);

	//Main PPU lifecycle methods
	void reset() override;
	void step(uint32_t cycles) override;

	//Memory-mapped I/O handlers for the MMU to call
	[[nodiscard]] uint8_t read_vram(uint16_t address) const final;
	void write_vram(uint16_t address, uint8_t value) final;
	[[nodiscard]] uint8_t read_oam(uint16_t addr) const final;
	void write_oam(uint16_t addr, uint8_t data) final;
	[[nodiscard]] uint8_t read_control(uint16_t addr) const final;
	void write_control(uint16_t addr, uint8_t data) final;

	//DMA transfer handling
	void start_dma_transfer() final;
	[[nodiscard]] bool is_dma_active() const final;

	[[nodiscard]] bool is_vram_accessible() const final;
	[[nodiscard]] bool is_oam_accessible() const final;

	//Interface for the frontend to get the final image
	[[nodiscard]] const std::array<ppu_types::rgba, 160 * 144>& get_framebuffer() const final;
private:
	void tick();
	void oam_scan();
	void increment_ly();
	void check_lyc_coincidence();
	void render_scanline();
	void render_bg(bool window);
	bool oam_render_possible();
	void render_oam();
	void render_window();
	uint16_t extract_tile_map_addr(bool fetching_window);
	[[nodiscard]] ppu_types::rgba get_color_from_palette(uint8_t color_id, uint8_t palette_reg) const;



	void fill_oam_buffer();
	void set_mode(ppu_types::ppu_mode new_mode);



	std::array<ppu_types::rgba, 160 * 144> framebuffer{};

	std::shared_ptr<shared::interrupt> interrupt_controller;

	uint8_t vram[8192]{};

	union {
		uint8_t oam[160]{};
		ppu_types::OAM_Sprite oam_sprites[40];
	};

	struct scanline_element {
		uint8_t color_id;
		uint8_t bgp;

	};



	//OAM Buffer
	std::array<ppu_fifo_types::OAM_priority_queue_element, 10>  sprite_buffer{};
	uint16_t sprite_buffer_index = 0;
	uint16_t current_sprite_index = 0 ;





	// PPU Registers using the types from ppu_types.h
	ppu_types::_lcd_control lcdc;
	ppu_types::_lcd_stat stat;
	uint8_t scy{};
	uint8_t scx{};
	uint8_t ly{};
	uint8_t lyc{};
	uint8_t bgp{};
	uint8_t obp0{};
	uint8_t obp1{};
	uint8_t wy{};
	uint8_t wx{};
	int32_t dma_cycles_remaining = 0;
	

	struct {
		ppu_types::ppu_mode current_mode;
		ppu_fifo_types::fifo_state background_fifo_state = ppu_fifo_types::fifo_state::GET_TILE;
		ppu_fifo_types::fifo_state sprite_fifo_state = ppu_fifo_types::fifo_state::GET_TILE;

		std::deque<ppu_fifo_types::fifo_element> background_fifo;
		std::deque<ppu_fifo_types::fifo_element> sprite_fifo;

		int current_pixel = 0;
		int bg_fetcher_cycle = 0;
		int total_dots = 0;
#pragma region Window
		uint16_t window_line = -1;
		bool window_triggered = false;
#pragma endregion

#pragma region Sprite
		ppu_types::OAM_Sprite current_sprite;
		ppu_types::line current_oam_line;
#pragma endregion
		bool pause_bg_fetch = false;
		ppu_types::line current_bg_line;

		uint8_t bg_tile_id = 0;

		int oam_cycle = 0;
		int current_x = 0;
		int first_fetch = 12;
		int discard_delay = 0;
		int discard_delay_set = false;
		int drawing_cycles = 0;

		[[nodiscard]] bool render_complete() const {
			return current_x > 160;
		}



		void hblank_reset() {
			oam_cycle = 0;
			current_x = 0;
			first_fetch = 12;
			current_pixel = 0;
			bg_tile_id = 0;

			window_triggered = false;
			background_fifo_state = ppu_fifo_types::fifo_state::GET_TILE;
			total_dots = 0;
			discard_delay = 0;
			discard_delay_set = false;
			pause_bg_fetch = false;
			drawing_cycles = 0;
			reset_bg_fifo();
			reset_sprite_fifo();

		}
		void vblank_reset()
		{
			window_line = -1;
			hblank_reset();
			
		}
		void reset_bg_fifo() {
			while (!background_fifo.empty()) background_fifo.pop_back();
		}
		void reset_sprite_fifo() {
			while (!sprite_fifo.empty()) sprite_fifo.pop_back();
		}

	} line_state;

	const std::array<ppu_types::rgba, 4> colors = {
	0xFFFFFFFF, // White
	0xC0C0C0FF, // Light gray
	0x606060FF, // Dark gray
	0x000000FF  // Black
	};
};
#endif