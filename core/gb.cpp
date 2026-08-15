#include "gb.h"

#include "ppu/scanline_ppu/ppu_scanline.h"
#include "ppu/tick_fifo_ppu/ppu_tick_fifo.h"
#include <timer/gb_timer.h>
#include <timer/gb_timer2.h>
#include <events/events.h>
#include <shared/hardware_constants.h>
//
// Created by Misael on 07/03/2025.
//

void gb::init()
{
}
gb::gb(const std::string& rompath, EmuFlags flags)
	: _flags(flags) {




	_interrupt_controller = std::make_shared<shared::interrupt>();

	// 2. Create the other components, passing the necessary shared resources.

	if (_flags.useFastPPU) {

		_ppu = std::make_shared<PPU_scanline>(_interrupt_controller);
	}
	else {
		_ppu = std::make_shared<ppu_tick_fifo>(_interrupt_controller);
	}

	//
	if (_flags.useNewTimer) {
		_timer = std::make_shared<gb_timer2>(_interrupt_controller);
	}
	else {
		_timer = std::make_shared<gb_timer>(_interrupt_controller);
	}

	_spu = std::make_shared<spu>(_interrupt_controller);

	_cartridge = std::move(Cartridge::get_cartridge(rompath));


	_mmu = std::make_shared<mmu::MMU>(_cartridge, _ppu, _timer, _interrupt_controller, _spu);

	_cpu = std::make_unique<::cpu::cpu>(_mmu, _interrupt_controller);



}
void gb::reset() {
	_cpu->reset();

}


void gb::run_one_frame() {
	uint32_t cycles_this_frame = 0;

	while (cycles_this_frame < gb_hardware::ppu::DotsPerFrame) {

		uint32_t spent_cycles = _cpu->step();

		if (this->_flags.useDotStepping) {
			for (uint32_t i = 0; i < spent_cycles; ++i) {
				_ppu->tick();
				_timer->tick();
				_spu->tick();
			}
		} else {
			_ppu->step(spent_cycles);
			_timer->step(spent_cycles);
			_spu->step(spent_cycles);

		}
		// 2. Update all other components by the exact same amount of time.


		// 3. Accumulate the cycles for this frame.
		cycles_this_frame += spent_cycles;
	
	}
	bus.send(FrameCompleteEvent{});
}

const std::array<ppu_types::rgba, gb_hardware::display::PixelCount>& gb::get_framebuffer() const {
	return _ppu->get_framebuffer();
}

std::vector<spu::stereo_sample> gb::consume_audio_samples() {
	return _spu->consume_samples();
}
