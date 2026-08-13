#include "spu.h"

namespace {
constexpr uint32_t CpuFrequency = 4194304;
constexpr uint32_t FrameSequencerPeriod = CpuFrequency / 512;
constexpr double CyclesPerSample = static_cast<double>(CpuFrequency) / spu::sample_rate;
}

spu::spu(std::shared_ptr<shared::interrupt> interrupts)
	: interrupts(std::move(interrupts))
{
	reset_audio_registers();
}

uint8_t spu::register_index(uint16_t addr) const
{
	return static_cast<uint8_t>(addr - AudioRegisterStart);
}

uint8_t spu::read_mask(uint16_t addr) const
{
	switch (addr) {
	case 0xFF10: return 0x80;
	case 0xFF11: return 0x3F;
	case 0xFF13: return 0xFF;
	case 0xFF14: return 0xBF;
	case 0xFF15: return 0xFF;
	case 0xFF16: return 0x3F;
	case 0xFF18: return 0xFF;
	case 0xFF19: return 0xBF;
	case 0xFF1A: return 0x7F;
	case 0xFF1B: return 0xFF;
	case 0xFF1C: return 0x9F;
	case 0xFF1D: return 0xFF;
	case 0xFF1E: return 0xBF;
	case 0xFF1F: return 0xFF;
	case 0xFF20: return 0xFF;
	case 0xFF23: return 0xBF;
	default: return 0x00;
	}
}

void spu::reset_audio_registers()
{
	registers.fill(0);
	powered_on = false;
	frame_sequencer_cycles = 0;
	frame_sequencer_step = 0;
	sample_cycles = 0.0;
	sample_buffer.clear();
}

uint8_t spu::read(uint16_t addr)
{
	if (addr == 0xFF26) {
		return static_cast<uint8_t>(0x70 | (powered_on ? 0x80 : 0x00));
	}

	return static_cast<uint8_t>(registers[register_index(addr)] | read_mask(addr));
}

uint8_t spu::read_wave(uint16_t addr)
{
	return wave_ram[addr - WaveRamStart];
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

	registers[register_index(addr)] = data;

	// Later implementation points:
	// - NRx1 writes load length timers.
	// - NRx2 writes configure DAC/envelope.
	// - NRx3/NRx4 writes configure period and trigger channels.
	// - NR50/NR51 configure final stereo volume and panning.
}

void spu::write_wave(uint16_t addr, uint8_t data)
{
	wave_ram[addr - WaveRamStart] = data;
}

void spu::tick()
{
	if (!powered_on) {
		return;
	}

	tick_channels();

	frame_sequencer_cycles++;
	if (frame_sequencer_cycles >= FrameSequencerPeriod) {
		frame_sequencer_cycles = 0;
		tick_frame_sequencer();
	}

	sample_cycles += 1.0;
	if (sample_cycles >= CyclesPerSample) {
		sample_cycles -= CyclesPerSample;
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
	// Silence for now. Replace this with:
	// 1. Read each channel's current 4-bit output.
	// 2. Apply NR51 panning.
	// 3. Apply NR50 left/right volume.
	// 4. Push the final stereo sample.
	sample_buffer.push_back({ 0.0f, 0.0f });
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
