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

gb::gb(const std::string& rompath,
       EmuFlags flags,
       std::shared_ptr<logging::CoreLogger> logger,
       std::shared_ptr<serial::GBSerial> serial)
	: _flags(flags), _serial(std::move(serial)), _logger(std::move(logger)) {

	if (_logger == nullptr) {
		_logger = std::make_shared<logging::NullCoreLogger>();
	}
	if (_serial == nullptr) {
		_serial = std::make_shared<serial::NullGBSerial>();
	}

	bus.subscribe<CartridgeLoadedEvent>([this](const CartridgeLoadedEvent& event) {
		_last_cartridge_loaded_event = event;
	});

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
	_joypad = std::make_shared<Joypad>(_interrupt_controller);

	_cartridge = std::move(Cartridge::get_cartridge(rompath, bus));


	_mmu = std::make_shared<mmu::MMU>(
		_cartridge, _ppu, _timer, _interrupt_controller, _spu, _serial, _joypad, _logger,
		_flags.useSlowReadPath);

	_cpu = std::make_unique<::cpu::cpu>(_mmu, _interrupt_controller, _logger);



}
void gb::reset() {
	_cpu->reset();

}

namespace {
	class NoopComponentObserver {
	public:
		void begin_frame() const {}
		void begin_component(profiling::GBComponent) const {}
		void end_component(profiling::GBComponent) const {}
		void end_frame() const {}
	};
}

template <typename ComponentObserver>
void gb::run_one_frame_with(ComponentObserver& observer) {
	uint32_t cycles_this_frame = 0;
	observer.begin_frame();

	while (cycles_this_frame < gb_hardware::ppu::DotsPerFrame) {
		observer.begin_component(profiling::GBComponent::Cpu);
		uint32_t spent_cycles = _cpu->step();
		observer.end_component(profiling::GBComponent::Cpu);

		if (this->_flags.useDotStepping) {
			for (uint32_t i = 0; i < spent_cycles; ++i) {
				observer.begin_component(profiling::GBComponent::Ppu);
				_ppu->tick();
				observer.end_component(profiling::GBComponent::Ppu);
				observer.begin_component(profiling::GBComponent::Timer);
				_timer->tick();
				observer.end_component(profiling::GBComponent::Timer);
				observer.begin_component(profiling::GBComponent::Spu);
				_spu->tick();
				observer.end_component(profiling::GBComponent::Spu);
			}
		} else {
			observer.begin_component(profiling::GBComponent::Ppu);
			_ppu->step(spent_cycles);
			observer.end_component(profiling::GBComponent::Ppu);
			observer.begin_component(profiling::GBComponent::Timer);
			_timer->step(spent_cycles);
			observer.end_component(profiling::GBComponent::Timer);
			observer.begin_component(profiling::GBComponent::Spu);
			_spu->step(spent_cycles);
			observer.end_component(profiling::GBComponent::Spu);

		}
		// 2. Update all other components by the exact same amount of time.


		// 3. Accumulate the cycles for this frame.
		cycles_this_frame += spent_cycles;
	
	}
	observer.end_frame();
	bus.send(FrameCompleteEvent{});
}

void gb::run_one_frame() {
	if (_component_observer != nullptr) {
		run_one_frame_with(*_component_observer);
		return;
	}

	NoopComponentObserver observer;
	run_one_frame_with(observer);
}

void gb::set_button(JoypadButton button, bool pressed) {
	_joypad->set_button(button, pressed);
}

const std::array<ppu_types::rgba, gb_hardware::display::PixelCount>& gb::get_framebuffer() const {
	return _ppu->get_framebuffer();
}

std::vector<spu::stereo_sample> gb::consume_audio_samples() {
	return _spu->consume_samples();
}
