
 //Created by Misael on 23/07/2025.


#ifndef PPU_H
#define PPU_H
#include "../../shared/interrupt.h"
#include "../types.h"
#include "../ppu_base.h"
#include <cstdint>
#include <memory>
#include <array>
#include <queue>




class PPU:public PPU_Base {
public:
     //Constructor takes a shared_ptr to the interrupt controller
    explicit PPU(std::shared_ptr<shared::interrupt> interrupt_controller);

     //Main PPU lifecycle methods
    void reset();
    void step(uint32_t cycles);

     //Memory-mapped I/O handlers for the MMU to call
    [[nodiscard]] uint8_t read_vram(uint16_t address) const;
    void write_vram(uint16_t address, uint8_t value);
    [[nodiscard]] uint8_t read_oam(uint16_t addr) const;
    void write_oam(uint16_t addr, uint8_t data);
    [[nodiscard]] uint8_t read_control(uint16_t addr) const;
    void write_control(uint16_t addr, uint8_t data);

     //DMA transfer handling
    void start_dma_transfer();
    [[nodiscard]] bool is_dma_active() const;

    [[nodiscard]] bool is_vram_accessible() const;
    [[nodiscard]] bool is_oam_accessible() const;

     //Interface for the frontend to get the final image
    [[nodiscard]] const std::array<ppu_types::rgba, 160 * 144>& get_framebuffer() const;



private:
     //Internal state machine and rendering logic
    void set_mode(ppu_types::ppu_mode new_mode);
    void increment_ly();
    void check_lyc_coincidence();
    void render_scanline(int cycles);
    void render_background(int cycles);
    void render_window(int cycles);
    void render_sprites(int cycles);
    void flush_to_buffer();

    [[nodiscard]] ppu_types::rgba get_color_from_palette(uint8_t color_id, uint8_t palette_reg) const;

     //Direct handle to the central interrupt controller
    std::shared_ptr<shared::interrupt> interrupt_controller;

     //PPU Memory
    uint8_t vram[8192];


    union {
        uint8_t oam[160];
        ppu_types::OAM_Sprite oam_sprites[40];
    };


     //Final image buffer
    std::array<ppu_types::rgba, 160 * 144> framebuffer{};
    uint8_t scanline_buffer[160]{}; //On finish, flush to framebuffer



     //PPU Registers using the types from ppu_types.h
#pragma region lcd_registers
    ppu_types::_lcd_control lcdc;
    ppu_types::_lcd_stat stat;

#pragma endregion

#pragma region ppu_registers
    uint8_t scy{};
    uint8_t scx{};
    uint8_t ly{};
    uint8_t lyc{};
    uint8_t bgp{};
    uint8_t obp0{};
    uint8_t obp1{};
    uint8_t wy{};
    uint8_t wx{};
#pragma endregion

#pragma region ppu_line_state
    struct {
        ppu_types::OAM_Sprite sprite_buffer[10];
        int sprite_buffer_count = 0;
        int hblank_cycles = 0;


        std::queue<uint8_t> background_fifo;

        int oam_dots{ 0 };
        bool oam_complete() {
            return oam_dots >= 80; //Simply check if its >80
        }

        int current_drawing_pixel = 0;
        bool drawing_complete = false;


        int hblank_dots{ 0 };

        void finish_line() {
            sprite_buffer_count = 0;
            hblank_dots = 0;
            oam_dots = 0;
            while (!background_fifo.empty()) background_fifo.pop();
                
        };
    } state;
#pragma endregion
     //Internal PPU State
    
    

    ppu_types::ppu_mode current_mode;
   
        
    void fill_oam_buffer();

    bool check_oam_fill_conditions(const ppu_types::OAM_Sprite& sprite, const int ly_plus_16, const int spriteheight);

    uint32_t cycle_counter{};
    
    int32_t dma_cycles_remaining {};
    int window_line_counter {};




     //The color palette
    const std::array<ppu_types::rgba, 4> colors = {
        0xFFFFFFFF, // White
        0xC0C0C0FF, // Light gray
        0x606060FF, // Dark gray
        0x000000FF  // Black
    };
};


#endif //PPU_H
