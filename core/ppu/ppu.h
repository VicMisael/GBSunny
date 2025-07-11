//
// Created by Misael on 12/03/2025.
//

#ifndef PPU_H
#define PPU_H
#include "../shared/interrupt.h"
#include "./types.h"
#include <cstdint>
#include <memory>
#include <array>




class PPU {
public:
    // Constructor takes a shared_ptr to the interrupt controller
    explicit PPU(std::shared_ptr<shared::interrupt> interrupt_controller);

    // Main PPU lifecycle methods
    void reset();
    void step(uint32_t cycles);

    // Memory-mapped I/O handlers for the MMU to call
    [[nodiscard]] uint8_t read_vram(uint16_t address) const;
    void write_vram(uint16_t address, uint8_t value);
    [[nodiscard]] uint8_t read_oam(uint16_t addr) const;
    void write_oam(uint16_t addr, uint8_t data);
    [[nodiscard]] uint8_t read_control(uint16_t addr) const;
    void write_control(uint16_t addr, uint8_t data);

    // DMA transfer handling
    void start_dma_transfer();
    [[nodiscard]] bool is_dma_active() const;

    [[nodiscard]] bool is_vram_accessible() const;
    [[nodiscard]] bool is_oam_accessible() const;

    // Interface for the frontend to get the final image
    [[nodiscard]] const std::array<ppu_types::rgba, 160 * 144>& get_framebuffer() const;



private:
    // Internal state machine and rendering logic
    void set_mode(ppu_types::ppu_mode new_mode);
    void increment_ly();
    void check_lyc_coincidence();
    void render_scanline();
    void render_background();
    void render_window();
    void render_sprites();
    [[nodiscard]] ppu_types::rgba get_color_from_palette(uint8_t color_id, uint8_t palette_reg) const;

    // Direct handle to the central interrupt controller
    std::shared_ptr<shared::interrupt> interrupt_controller;

    // PPU Memory
    //std::array<uint8_t, 8192> vram;
    uint8_t vram[8192];
    uint8_t oam[160];

    // Final image buffer
    std::array<ppu_types::rgba, 160 * 144> framebuffer{};
    uint8_t scanline_buffer[160]{};

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

    // Internal PPU State
    ppu_types::ppu_mode current_mode;
    uint32_t cycle_counter{};
    int32_t dma_cycles_remaining {};
    int window_line_counter {};

    // The color palette
    const std::array<ppu_types::rgba, 4> colors;
};


#endif //PPU_H
