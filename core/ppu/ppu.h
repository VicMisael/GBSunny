//
// Created by Misael on 12/03/2025.
//

#ifndef PPU_H
#define PPU_H
#include "../shared/interrupt.h"
#include "types.h"
#include <cstdint>
#include <memory>
#include <utility>




class PPU {
    uint8_t video_RAM[8192] = {};

    ppu_types::_lcd_control lcd_control;

    uint8_t LY;
    uint8_t LYC;

    std::shared_ptr<shared::interrupt> interrupt; //Shared space for interrupts

    //OAM
    int32_t oam_transfer_remaining_cycles;
    void start_oam_transfer();

public:

    explicit PPU(std::shared_ptr<shared::interrupt> interrupt) : interrupt(std::move(interrupt)), lcd_control(0) {}

    bool is_oam_transfer_running() const;

    void reset();
    [[nodiscard]] uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t value);
    [[nodiscard]] uint8_t read_oam(uint16_t addr) const;

    [[nodiscard]] uint8_t read_ppucontrol(uint16_t addr) const;
    void write_ppucontrol(uint16_t addr, uint8_t data);

    void write_oam(uint16_t addr, uint8_t data);

    void step(const uint8_t cycles_to_run);
};



#endif //PPU_H
