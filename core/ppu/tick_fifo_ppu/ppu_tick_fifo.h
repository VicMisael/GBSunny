#ifndef PPU_TICK_FIFO_H
#define PPU_TICK_FIFO_H
#include "../ppu_base.h"
#include "shared/interrupt.h"
#include "ppu_fifo_types.h"
#include <memory>
#include "utils/utils.h"


class ppu_tick_fifo :public PPU_Base {
public:
	//Constructor takes a shared_ptr to the interrupt controller
	explicit ppu_tick_fifo(std::shared_ptr<shared::interrupt> interrupt_controller);

	//Main PPU lifecycle methods
	void reset() override;
	void step(uint32_t cycles) override;
	void tick() override;

	//Memory-mapped I/O handlers for the MMU to call
	[[nodiscard]] uint8_t read_vram(uint16_t address) const final;
	void write_vram(uint16_t address, uint8_t value) final;
	[[nodiscard]] const uint8_t* get_vram_ptr() const final { return vram.data(); }
	[[nodiscard]] uint8_t read_control(uint16_t addr) const final;
	void write_control(uint16_t addr, uint8_t data) final;

	//DMA transfer handling
	void start_dma_transfer() final;
	[[nodiscard]] bool is_dma_active() const final;

	[[nodiscard]] bool is_oam_accessible() const final;
	[[nodiscard]] bool is_vram_accessible() const final;
	//Interface for the frontend to get the final image
    [[nodiscard]] const std::array<ppu_types::rgba, gb_hardware::display::PixelCount>& get_framebuffer() const final;
private:
	void lock_vram_access();
	void unlock_vram_access();

	void scanline_checks();
	void oam_scan();
	void increment_ly();
	void check_lyc_coincidence();
	void render_scanline();
	void render_bg(bool window);
	[[nodiscard]] bool oam_render_possible() const;
	void render_oam();
	[[nodiscard]] ppu_types::rgba render_oam_pixel(const ppu_fifo_types::fifo_element& bg, ppu_types::rgba color) const;
	void reset_lcd_state();
	[[nodiscard]] bool stat_interrupt_signal() const;
	void update_stat_interrupt_line();
	[[nodiscard]] uint16_t extract_tile_map_addr(bool fetching_window) const;
	[[nodiscard]] ppu_types::rgba get_color_from_palette(uint8_t color_id, uint8_t palette_reg) const;


	void fill_oam_buffer();
	void set_mode(ppu_types::ppu_mode new_mode);

	[[nodiscard]] uint8_t read_vram_internal(uint16_t addr) const;

	void write_vram_internal(uint16_t addr, uint8_t data);


	std::array<ppu_types::rgba, gb_hardware::display::PixelCount> framebuffer{};

	std::shared_ptr<shared::interrupt> interrupt_controller;

	std::array<uint8_t,8192> vram{};

	bool vram_accessible = true;
	struct scanline_element {
		uint8_t color_id;
		uint8_t bgp;

	};




	//OAM Buffer
	ppu_fifo_types::oam_ring_buffer<10>  sprite_buffer{};









	struct {

		ppu_fifo_types::fifo_state background_fifo_state = ppu_fifo_types::fifo_state::GET_TILE;
		ppu_fifo_types::fifo_state sprite_fifo_state = ppu_fifo_types::fifo_state::GET_TILE;

		utils::FixedDeque<ppu_fifo_types::fifo_element> background_fifo;
		utils::FixedDeque<ppu_fifo_types::fifo_element> sprite_fifo;

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
		ppu_types::line current_bg_line;

		uint8_t bg_tile_id = 0;

		int oam_cycle = 0;
		int current_x = 0;

		int drawing_cycles = 0;

		bool bg_fetcher_running = false;
		bool oam_fetcher_running = false;
		bool window_ly_equals_wy = false;


		[[nodiscard]] bool render_complete() const {
			return current_x > gb_hardware::display::Width;
		}



		void hblank_reset() {
			oam_cycle = 0;
			current_x = 0;
			current_pixel = 0;
			bg_tile_id = 0;
			bg_fetcher_running = false;
			oam_fetcher_running = false;
			window_triggered = false;
			background_fifo_state = ppu_fifo_types::fifo_state::GET_TILE;
			sprite_fifo_state = ppu_fifo_types::fifo_state::GET_TILE;
			total_dots = 0;

			drawing_cycles = 0;
			reset_bg_fifo();
			reset_sprite_fifo();

		}
		void vblank_reset()
		{
			window_line = -1;
			window_ly_equals_wy = false;
			hblank_reset();

		}
		void reset_bg_fifo() {
			background_fifo.clear();
		}
		void reset_sprite_fifo() {
			sprite_fifo.clear();
		}

	} state;

	bool stat_interrupt_line = false;

};
#endif
