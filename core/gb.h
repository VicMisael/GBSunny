//
// Created by Misael on 07/03/2025.
//

#ifndef GB_H
#define GB_H
#include "cpu/cpu.h"
#include "spu/spu.h"
#include <ppu/ppu_base.h>

class gb {
    std::shared_ptr<Cartridge> _cartridge;
    std::shared_ptr<shared::interrupt> _interrupt_controller;
    std::shared_ptr<PPU_Base> _ppu;
    std::shared_ptr<base_timer> _timer;
    std::unique_ptr<cpu::cpu> _cpu;
    std::shared_ptr<spu> _spu;
    

    void init();
public:
    std::shared_ptr<mmu::MMU> _mmu;
    explicit gb(const std::string& rompath,bool fast_ppu=true);
    void reset();
    void run_one_frame();
    [[nodiscard]] const std::array<ppu_types::rgba, 160 * 144>& get_framebuffer() const;




};



#endif //GB_H
