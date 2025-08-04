#include "gb.h"

#include "ppu/scanline_ppu/ppu_scanline.h"
#include "ppu/tick_fifo_ppu/ppu_tick_fifo.h"
//
// Created by Misael on 07/03/2025.
//

void gb::init()
{
}
gb::gb(const std::string &rompath,bool fast_ppu) {
  _interrupt_controller = std::make_shared<shared::interrupt>();

  // 2. Create the other components, passing the necessary shared resources.

  if (fast_ppu) {

      _ppu = std::make_shared<PPU_scanline>(_interrupt_controller);
  }
  else {
      _ppu = std::make_shared<ppu_tick_fifo>(_interrupt_controller);
  }

  //
  _timer = std::make_shared<gb_timer>(_interrupt_controller);

  _spu = std::make_shared<spu>();

  _timer = std::make_shared<gb_timer>(_interrupt_controller);
  _cartridge = std::move(Cartridge::get_cartridge(rompath));
  _mmu =  std::make_shared<mmu::MMU>(_cartridge,_ppu,_timer,_interrupt_controller,_spu);

  _cpu = std::make_unique<::cpu::cpu>(_mmu,_interrupt_controller);


}
void gb::reset() {
  _cpu->reset();

}


void gb::run_one_frame() {
  //const int CYCLES_PER_FRAME = 69905;
  
  const int CYCLES_PER_FRAME = 70224;
  int cycles_this_frame = 0;

  while (cycles_this_frame < CYCLES_PER_FRAME) {

    uint32_t spent_cycles = _cpu->step(); 

    // 2. Update all other components by the exact same amount of time.
    _ppu->step(spent_cycles);
    _timer->step(spent_cycles);
    // spu->step(spent_cycles); // Add this if your SPU needs cycle-based updates

    // 3. Accumulate the cycles for this frame.
    cycles_this_frame += spent_cycles;
  }

}

const std::array<ppu_types::rgba, 160 * 144>& gb::get_framebuffer() const{
  return _ppu->get_framebuffer();
}

