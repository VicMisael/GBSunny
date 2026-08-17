//
// Created by Misael on 07/03/2025.
//

#ifndef GB_H
#define GB_H
#include "cpu/cpu.h"
#include "joypad/joypad.h"
#include "spu/spu.h"
#include "utils/emu_flags.h"
#include "serial/gb_serial.h"
#include <ppu/ppu_base.h>
#include <events/event_aggregator.h>
#include <events/events.h>
#include <shared/hardware_constants.h>
#include <vector>

class gb {
    EmuFlags _flags;
    std::shared_ptr<Cartridge> _cartridge;
    std::shared_ptr<shared::interrupt> _interrupt_controller;
    std::shared_ptr<PPU_Base> _ppu;
    std::shared_ptr<base_timer> _timer;
    std::unique_ptr<cpu::cpu> _cpu;
    std::shared_ptr<spu> _spu;
    std::shared_ptr<serial::GBSerial> _serial;
    std::shared_ptr<Joypad> _joypad;


    void init();
public:
    std::shared_ptr<mmu::MMU> _mmu;
    EmulatorEventAggregator bus;
    explicit gb(const std::string& rompath, EmuFlags flags = { false, true, true });
    void reset();
    void run_one_frame();
    void set_button(JoypadButton button, bool pressed);
    [[nodiscard]] const std::array<ppu_types::rgba, gb_hardware::display::PixelCount>& get_framebuffer() const;
    std::vector<spu::stereo_sample> consume_audio_samples();




};



#endif //GB_H
