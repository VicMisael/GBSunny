#include "spu.h"
#include <cmath>
#include "shared/hardware_constants.h"

spu::spu(std::shared_ptr<shared::interrupt> interrupts)
	: interrupts(std::move(interrupts))
{
	reset_audio_registers();
}

void spu::reset_audio_registers()
{
	registers.reset();
	powered_on = false;
	frame_sequencer_cycles = 0;
	frame_sequencer_step = 0;
	sample_cycles = 0.0;
	sample_buffer.clear();
}

uint8_t spu::read(uint16_t addr) const
{
	return registers.read(addr, powered_on);
}

uint8_t spu::read_wave(uint16_t addr) const
{
	return registers.read_wave(addr);
}

void spu::write(uint16_t addr, uint8_t data)
{
	if (addr == 0xFF26) {
		if ((data & 0x80) == 0) {
			reset_audio_registers();
		}
		else {
			powered_on = true;
		}
		return;
	}

	if (!powered_on || addr == 0xFF15 || addr == 0xFF1F) {
		return;
	}

	registers.write(addr, data);
}

void spu::write_wave(uint16_t addr, uint8_t data)
{
	registers.write_wave(addr, data);
}

void spu::tick()
{
	if (!powered_on) {
		return;
	}

	tick_channels();

	frame_sequencer_cycles++;
	if (frame_sequencer_cycles >= gb_hardware::apu::FrameSequencerPeriod) {
		frame_sequencer_cycles = 0;
		tick_frame_sequencer();
	}

	sample_cycles += 1.0;
	if (sample_cycles >= gb_hardware::apu::CyclesPerSample) {
		sample_cycles -= gb_hardware::apu::CyclesPerSample;
		mix_sample();
	}
}

void spu::tick_frame_sequencer()
{
	// Runs at 512 Hz.
	// Step 0/2/4/6: length timers.
	// Step 2/6: channel 1 sweep.
	// Step 7: volume envelopes.
	frame_sequencer_step = (frame_sequencer_step + 1) & 0x07;
}

void spu::tick_channels()
{
	// This is where the four Game Boy channels will advance their internal
	// waveform timers:
	// - CH1 pulse + sweep
	// - CH2 pulse
	// - CH3 wave RAM playback
	// - CH4 noise LFSR
}

void spu::mix_sample()
{
	static double phase = 0.0;

	constexpr double frequency = 440.0;
	constexpr double amplitude = 0.2;

	constexpr double twoPi = 6.28318530717958647692;

	const double phaseIncrement =
		twoPi * frequency / gb_hardware::apu::SampleRate;

	const float sample =
		static_cast<float>(std::sin(phase) * amplitude);

	phase += phaseIncrement;

	if (phase >= twoPi)
		phase -= twoPi;

	sample_buffer.push_back({ sample, sample });
}

void spu::step(uint32_t cycles)
{
	for (uint32_t i = 0; i < cycles; i++) {
		tick();
	}
}

std::vector<spu::stereo_sample> spu::consume_samples()
{
	std::vector<stereo_sample> samples;
	samples.swap(sample_buffer);
	return samples;
}
