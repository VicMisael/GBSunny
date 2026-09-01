#include "spu.h"

#include "shared/hardware_constants.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace {
constexpr uint16_t PulseLengthMaximum = 64;
constexpr uint16_t WaveLengthMaximum = 256;

// The patterns are indexed by the two duty bits in NR11/NR21 and by each
// pulse channel's private three-bit duty-position counter.
constexpr std::array<std::array<uint8_t, 8>, 4> DutyPatterns{{
	{{0, 0, 0, 0, 0, 0, 0, 1}}, // 12.5%
	{{1, 0, 0, 0, 0, 0, 0, 1}}, // 25%
	{{1, 0, 0, 0, 0, 1, 1, 1}}, // 50%
	{{0, 1, 1, 1, 1, 1, 1, 0}}, // 75%
}};
}

spu::spu(std::shared_ptr<shared::interrupt> interrupts)
	: interrupts(std::move(interrupts))
{
	reset_audio_registers();
}

void spu::reset_audio_registers()
{
	registers.reset();
	channel1 = {};
	channel2 = {};
	channel1_sweep = {};
	channel3 = {};
	channel4 = {};
	powered_on = false;
	frame_sequencer_cycles = 0;
	frame_sequencer_step = 0;
	sample_clock_accumulator = 0;
	left_filter_capacitor = 0.0f;
	right_filter_capacitor = 0.0f;
	sample_buffer.clear();
}

void spu::power_off()
{
	// Wave RAM deliberately survives this reset, matching NR52 power-off
	// behavior. The DIV-derived frame-sequencer phase also keeps running.
	registers.reset();
	channel1 = {};
	channel2 = {};
	channel1_sweep = {};
	channel3 = {};
	channel4 = {};
	powered_on = false;
	left_filter_capacitor = 0.0f;
	right_filter_capacitor = 0.0f;
	sample_buffer.clear();
}

uint8_t spu::read(uint16_t addr) const
{
	return registers.read(addr, powered_on, channel_status());
}

uint8_t spu::read_wave(uint16_t addr) const
{
	return registers.read_wave(addr);
}

void spu::write(uint16_t addr, uint8_t data)
{
	if (addr == 0xFF26) {
		if ((data & 0x80) == 0) {
			power_off();
		}
		else {
			powered_on = true;
		}
		return;
	}

	if (!powered_on || addr == 0xFF15 || addr == 0xFF1F) {
		return;
	}

	const uint8_t old_sweep = registers.channel1.sweep();
	registers.write(addr, data);

	switch (addr) {
	case 0xFF10:
		// Once subtraction has been used, changing the sweep direction back
		// to addition disables CH1 on the hardware.
		if ((old_sweep & 0x08) != 0
			&& (data & 0x08) == 0
			&& channel1_sweep.subtraction_used) {
			channel1.enabled = false;
		}
		break;

	case 0xFF11:
		channel1.length_counter = PulseLengthMaximum - (data & 0x3F);
		break;
	case 0xFF12:
		if (!envelope_dac_enabled(data)) {
			channel1.enabled = false;
		}
		break;
	case 0xFF14:
		if ((data & 0x80) != 0) {
			trigger_channel1();
		}
		break;

	case 0xFF16:
		channel2.length_counter = PulseLengthMaximum - (data & 0x3F);
		break;
	case 0xFF17:
		if (!envelope_dac_enabled(data)) {
			channel2.enabled = false;
		}
		break;
	case 0xFF19:
		if ((data & 0x80) != 0) {
			trigger_channel2();
		}
		break;

	case 0xFF1A:
		if ((data & 0x80) == 0) {
			channel3.enabled = false;
		}
		break;
	case 0xFF1B:
		channel3.length_counter = WaveLengthMaximum - data;
		break;
	case 0xFF1E:
		if ((data & 0x80) != 0) {
			trigger_channel3();
		}
		break;

	case 0xFF20:
		channel4.length_counter = PulseLengthMaximum - (data & 0x3F);
		break;
	case 0xFF21:
		if (!envelope_dac_enabled(data)) {
			channel4.enabled = false;
		}
		break;
	case 0xFF23:
		if ((data & 0x80) != 0) {
			trigger_channel4();
		}
		break;

	default:
		break;
	}
}

void spu::write_wave(uint16_t addr, uint8_t data)
{
	registers.write_wave(addr, data);
}

void spu::step(uint32_t cycles)
{
	return;
	for (uint32_t cycle = 0; cycle < cycles; ++cycle) {
		tick();
	}
}

void spu::tick()
{
	if (powered_on) {
		tick_channels();
	}

	// This approximates DIV-APU with a fixed master-clock divider. Its phase
	// continues while the APU is powered off, as the hardware DIV counter does.
	++frame_sequencer_cycles;
	if (frame_sequencer_cycles >= gb_hardware::apu::FrameSequencerPeriod) {
		frame_sequencer_cycles -= gb_hardware::apu::FrameSequencerPeriod;
		tick_frame_sequencer();
	}

	// Integer phase accumulation produces exactly SampleRate samples for every
	// CpuClockHz T-cycles without accumulating floating-point timing error.
	sample_clock_accumulator += gb_hardware::apu::SampleRate;
	if (sample_clock_accumulator >= gb_hardware::CpuClockHz) {
		sample_clock_accumulator -= gb_hardware::CpuClockHz;
		mix_sample();
	}
}

void spu::tick_frame_sequencer()
{
	if (powered_on) {
		switch (frame_sequencer_step) {
		case 0:
		case 4:
			tick_length_counters();
			break;
		case 2:
		case 6:
			tick_length_counters();
			tick_sweep();
			break;
		case 7:
			tick_envelope(channel1.envelope);
			tick_envelope(channel2.envelope);
			tick_envelope(channel4.envelope);
			break;
		default:
			break;
		}
	}

	frame_sequencer_step = (frame_sequencer_step + 1) & 0x07;
}

void spu::tick_channels()
{
	tick_pulse(channel1, channel1_period());
	tick_pulse(channel2, channel2_period());
	tick_wave();
	tick_noise();
}

void spu::tick_length_counters()
{
	auto tick_length = [](bool length_enabled, auto& channel) {
		if (!length_enabled || channel.length_counter == 0) {
			return;
		}

		--channel.length_counter;
		if (channel.length_counter == 0) {
			channel.enabled = false;
		}
	};

	tick_length(
		(registers.channel1.period_high_control() & 0x40) != 0,
		channel1);
	tick_length(
		(registers.channel2.period_high_control() & 0x40) != 0,
		channel2);
	tick_length(
		(registers.channel3.period_high_control() & 0x40) != 0,
		channel3);
	tick_length(
		(registers.channel4.control() & 0x40) != 0,
		channel4);
}

void spu::trigger_envelope(envelope_state& envelope, uint8_t nrx2)
{
	envelope.volume = nrx2 >> 4;
	envelope.pace = nrx2 & 0x07;
	envelope.timer = envelope.pace == 0 ? 8 : envelope.pace;
	envelope.increase = (nrx2 & 0x08) != 0;
	envelope.running = envelope.pace != 0;
}

void spu::tick_envelope(envelope_state& envelope)
{
	if (!envelope.running) {
		return;
	}

	if (envelope.timer > 0) {
		--envelope.timer;
	}
	if (envelope.timer != 0) {
		return;
	}

	envelope.timer = envelope.pace == 0 ? 8 : envelope.pace;
	if (envelope.increase && envelope.volume < 15) {
		++envelope.volume;
	}
	else if (!envelope.increase && envelope.volume > 0) {
		--envelope.volume;
	}
	else {
		envelope.running = false;
	}
}

void spu::tick_sweep()
{
	if (!channel1_sweep.enabled) {
		return;
	}

	if (channel1_sweep.timer > 0) {
		--channel1_sweep.timer;
	}
	if (channel1_sweep.timer != 0) {
		return;
	}

	const uint8_t sweep = registers.channel1.sweep();
	const uint8_t pace = (sweep >> 4) & 0x07;
	channel1_sweep.timer = pace == 0 ? 8 : pace;

	if (pace == 0 || (sweep & 0x07) == 0) {
		return;
	}

	const uint16_t new_period = calculate_sweep_period(true);
	if (new_period <= 0x07FF) {
		calculate_sweep_period(false);
	}
}

uint16_t spu::calculate_sweep_period(bool update)
{
	const uint8_t sweep = registers.channel1.sweep();
	const uint8_t shift = sweep & 0x07;
	const uint16_t delta = channel1_sweep.shadow_period >> shift;
	uint16_t new_period = 0;

	if ((sweep & 0x08) != 0) {
		new_period = channel1_sweep.shadow_period - delta;
		channel1_sweep.subtraction_used = true;
	}
	else {
		new_period = channel1_sweep.shadow_period + delta;
	}

	if (new_period > 0x07FF) {
		channel1.enabled = false;
		return new_period;
	}

	if (update && shift != 0) {
		channel1_sweep.shadow_period = new_period;
		registers.channel1.write(3, static_cast<uint8_t>(new_period));
		registers.channel1.write(
			4,
			static_cast<uint8_t>(
				(registers.channel1.period_high_control() & 0xF8)
				| ((new_period >> 8) & 0x07)));
	}

	return new_period;
}

void spu::tick_pulse(pulse_channel_state& channel, uint16_t period_value)
{
	if (!channel.enabled) {
		return;
	}

	if (channel.period_timer > 0) {
		--channel.period_timer;
	}
	if (channel.period_timer == 0) {
		channel.period_timer = static_cast<uint32_t>((2048 - period_value) * 4);
		channel.duty_position = (channel.duty_position + 1) & 0x07;
	}
}

void spu::tick_wave()
{
	if (!channel3.enabled) {
		return;
	}

	if (channel3.period_timer > 0) {
		--channel3.period_timer;
	}
	if (channel3.period_timer == 0) {
		channel3.period_timer = static_cast<uint32_t>((2048 - channel3_period()) * 2);
		channel3.sample_position = (channel3.sample_position + 1) & 0x1F;
	}
}

void spu::tick_noise()
{
	if (!channel4.enabled) {
		return;
	}

	const uint32_t timer_period = channel4_timer_period();
	if (timer_period == 0) {
		return;
	}

	if (channel4.period_timer > 0) {
		--channel4.period_timer;
	}
	if (channel4.period_timer != 0) {
		return;
	}

	channel4.period_timer = timer_period;
	const uint16_t feedback = (channel4.lfsr ^ (channel4.lfsr >> 1)) & 0x01;
	channel4.lfsr = static_cast<uint16_t>((channel4.lfsr >> 1) | (feedback << 14));

	if ((registers.channel4.frequency_randomness() & 0x08) != 0) {
		channel4.lfsr = static_cast<uint16_t>(
			(channel4.lfsr & ~(1U << 6)) | (feedback << 6));
	}
}

void spu::trigger_channel1()
{
	channel1.enabled = envelope_dac_enabled(registers.channel1.volume_envelope());
	if (channel1.length_counter == 0) {
		channel1.length_counter = PulseLengthMaximum;
	}
	channel1.period_timer = static_cast<uint32_t>((2048 - channel1_period()) * 4);
	trigger_envelope(channel1.envelope, registers.channel1.volume_envelope());

	channel1_sweep.shadow_period = channel1_period();
	const uint8_t sweep = registers.channel1.sweep();
	const uint8_t pace = (sweep >> 4) & 0x07;
	channel1_sweep.timer = pace == 0 ? 8 : pace;
	channel1_sweep.enabled = pace != 0 || (sweep & 0x07) != 0;
	channel1_sweep.subtraction_used = false;

	if ((sweep & 0x07) != 0) {
		calculate_sweep_period(false);
	}
}

void spu::trigger_channel2()
{
	channel2.enabled = envelope_dac_enabled(registers.channel2.volume_envelope());
	if (channel2.length_counter == 0) {
		channel2.length_counter = PulseLengthMaximum;
	}
	channel2.period_timer = static_cast<uint32_t>((2048 - channel2_period()) * 4);
	trigger_envelope(channel2.envelope, registers.channel2.volume_envelope());
}

void spu::trigger_channel3()
{
	channel3.enabled = (registers.channel3.dac_enable() & 0x80) != 0;
	if (channel3.length_counter == 0) {
		channel3.length_counter = WaveLengthMaximum;
	}
	channel3.period_timer = static_cast<uint32_t>((2048 - channel3_period()) * 2);
	channel3.sample_position = 0;
}

void spu::trigger_channel4()
{
	channel4.enabled = envelope_dac_enabled(registers.channel4.volume_envelope());
	if (channel4.length_counter == 0) {
		channel4.length_counter = PulseLengthMaximum;
	}
	channel4.period_timer = channel4_timer_period();
	channel4.lfsr = 0x7FFF;
	trigger_envelope(channel4.envelope, registers.channel4.volume_envelope());
}

uint16_t spu::channel1_period() const
{
	return static_cast<uint16_t>(
		registers.channel1.period_low()
		| ((registers.channel1.period_high_control() & 0x07) << 8));
}

uint16_t spu::channel2_period() const
{
	return static_cast<uint16_t>(
		registers.channel2.period_low()
		| ((registers.channel2.period_high_control() & 0x07) << 8));
}

uint16_t spu::channel3_period() const
{
	return static_cast<uint16_t>(
		registers.channel3.period_low()
		| ((registers.channel3.period_high_control() & 0x07) << 8));
}

uint32_t spu::channel4_timer_period() const
{
	const uint8_t randomness = registers.channel4.frequency_randomness();
	const uint8_t divisor_code = randomness & 0x07;
	const uint8_t clock_shift = randomness >> 4;

	// On DMG hardware shifts 14 and 15 do not clock the LFSR.
	if (clock_shift >= 14) {
		return 0;
	}

	const uint32_t divisor = divisor_code == 0 ? 8 : divisor_code * 16;
	return divisor << clock_shift;
}

bool spu::envelope_dac_enabled(uint8_t nrx2)
{
	return (nrx2 & 0xF8) != 0;
}

uint8_t spu::pulse_output(
	const pulse_channel_state& channel,
	uint8_t duty) const
{
	return DutyPatterns[duty & 0x03][channel.duty_position] != 0
		? channel.envelope.volume
		: 0;
}

uint8_t spu::wave_output() const
{
	const uint16_t address = static_cast<uint16_t>(
		SpuRegisterFile::WaveRamStart + channel3.sample_position / 2);
	const uint8_t sample_byte = registers.read_wave(address);
	const uint8_t sample = (channel3.sample_position & 1) == 0
		? sample_byte >> 4
		: sample_byte & 0x0F;

	switch ((registers.channel3.output_level() >> 5) & 0x03) {
	case 0: return 0;
	case 1: return sample;
	case 2: return sample >> 1;
	case 3: return sample >> 2;
	default: return 0;
	}
}

uint8_t spu::noise_output() const
{
	return (channel4.lfsr & 0x01) == 0
		? channel4.envelope.volume
		: 0;
}

uint8_t spu::channel_status() const
{
	return static_cast<uint8_t>(
		(channel1.enabled ? 0x01 : 0x00)
		| (channel2.enabled ? 0x02 : 0x00)
		| (channel3.enabled ? 0x04 : 0x00)
		| (channel4.enabled ? 0x08 : 0x00));
}

float spu::high_pass(float input, float& capacitor) const
{
	// 0.999958 is the DMG capacitor factor per master-clock cycle. Convert it
	// to the host sample interval so the filter cutoff is sample-rate neutral.
	static const float charge_factor = static_cast<float>(std::pow(
		0.999958,
		gb_hardware::apu::CyclesPerSample));
	const float output = input - capacitor;
	capacitor = input - output * charge_factor;
	return output;
}

void spu::mix_sample()
{
	if (!powered_on) {
		sample_buffer.push_back({0.0f, 0.0f});
		return;
	}

	const std::array<uint8_t, 4> digital_outputs{{
		pulse_output(channel1, registers.channel1.length_duty() >> 6),
		pulse_output(channel2, registers.channel2.length_duty() >> 6),
		wave_output(),
		noise_output(),
	}};
	const std::array<bool, 4> dac_outputs_connected{{
		channel1.enabled && envelope_dac_enabled(registers.channel1.volume_envelope()),
		channel2.enabled && envelope_dac_enabled(registers.channel2.volume_envelope()),
		channel3.enabled && (registers.channel3.dac_enable() & 0x80) != 0,
		channel4.enabled && envelope_dac_enabled(registers.channel4.volume_envelope()),
	}};

	const uint8_t panning = registers.global.sound_panning();
	float left = 0.0f;
	float right = 0.0f;
	for (std::size_t channel = 0; channel < digital_outputs.size(); ++channel) {
		if (!dac_outputs_connected[channel]) {
			continue;
		}

		// The Game Boy DAC has negative polarity: digital 0 maps to +1 and
		// digital 15 maps to -1.
		const float analog = 1.0f - static_cast<float>(digital_outputs[channel]) / 7.5f;
		if ((panning & (1U << channel)) != 0) {
			right += analog;
		}
		if ((panning & (1U << (channel + 4))) != 0) {
			left += analog;
		}
	}

	const uint8_t volume = registers.global.master_volume_vin();
	const float right_volume = static_cast<float>((volume & 0x07) + 1) / 8.0f;
	const float left_volume = static_cast<float>(((volume >> 4) & 0x07) + 1) / 8.0f;
	right = high_pass((right * right_volume) / 4.0f, right_filter_capacitor);
	left = high_pass((left * left_volume) / 4.0f, left_filter_capacitor);

	sample_buffer.push_back({
		std::clamp(left, -1.0f, 1.0f),
		std::clamp(right, -1.0f, 1.0f),
	});
}

std::vector<spu::stereo_sample> spu::consume_samples()
{
	std::vector<stereo_sample> samples;
	samples.swap(sample_buffer);
	return samples;
}
