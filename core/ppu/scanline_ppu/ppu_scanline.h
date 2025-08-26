//
// Created by Misael on 12/03/2025.
//

#ifndef PPU_H
#define PPU_H
#include "../../shared/interrupt.h"
#include "../types.h"
#include "../ppu_base.h"
#include <cstdint>
#include <memory>
#include <array>




class PPU_scanline : public PPU_Base {
public:
    // Constructor takes a shared_ptr to the interrupt controller
    explicit PPU_scanline(std::shared_ptr<shared::interrupt> interrupt_controller);

    // Main PPU lifecycle methods
    void reset() override;
    void step(uint32_t cycles) override;

    // Memory-mapped I/O handlers for the MMU to call
    [[nodiscard]] uint8_t read_vram(uint16_t address) const override;
    void write_vram(uint16_t address, uint8_t value) override;
    [[nodiscard]] uint8_t read_oam(uint16_t addr) const override;
    void write_oam(uint16_t addr, uint8_t data) override;
    [[nodiscard]] uint8_t read_control(uint16_t addr) const final;
    void write_control(uint16_t addr, uint8_t data) override ;

    // DMA transfer handling
    void start_dma_transfer() final;
    [[nodiscard]] bool is_dma_active() const final;

    [[nodiscard]] bool is_vram_accessible() const final;
    [[nodiscard]] bool is_oam_accessible() const final;

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


    void fill_oam_buffer();




    // Direct handle to the central interrupt controller
    std::shared_ptr<shared::interrupt> interrupt_controller;

    // PPU Memory
    //std::array<uint8_t, 8192> vram;
    uint8_t vram[8192]{};

    union {
        uint8_t oam[160]{};
        ppu_types::OAM_Sprite oam_sprites[40];
    };



    // Final image buffer
    std::array<ppu_types::rgba, 160 * 144> framebuffer{};

    struct scanline_element {
        uint8_t color_id;
        uint8_t bgp;

    };


    scanline_element scanline_buffer[160]{};

    //OAM Buffer
    std::array<ppu_types::OAM_Sprite,10> sprite_buffer{};
    int sprite_buffer_index = 0;

    // PPU Registers using the types from ppu_types.h

    // Internal PPU State
    ppu_types::ppu_mode current_mode;
    uint32_t cycle_counter{};
    int32_t dma_cycles_remaining {};
    int window_line_counter {};
    void scanline_checks();

    struct
    {
        bool window_triggered = false;
        void hblank_reset(){}
        void vblank_reset()
        {
            window_triggered = false;
            hblank_reset();
        }
    } state;
};


#endif //PPU_H
