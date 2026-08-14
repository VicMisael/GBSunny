//
// Created by visael on 14/03/25.
//

#ifndef SPU_H
#define SPU_H

#include <cstdint>
#include <memory>
#include <shared/interrupt.h>
#include <vector>

#include "SpuRegisterFile.h"

class spu {
public:
	struct stereo_sample {
		float left;
		float right;
	};

	explicit spu(std::shared_ptr<shared::interrupt> interrupts);

	[[nodiscard]] uint8_t read(uint16_t addr) const;
	[[nodiscard]] uint8_t read_wave(uint16_t addr) const;
	void write(uint16_t addr, uint8_t data);
	void write_wave(uint16_t addr, uint8_t data);

	void step(uint32_t cycles);
	void tick();
	std::vector<stereo_sample> consume_samples();

private:
	struct envelope_state {
		uint8_t volume = 0;
		uint8_t timer = 0;
		uint8_t pace = 0;
		bool increase = false;
		bool running = false;
	};

	struct pulse_channel_state {
		bool enabled = false;
		uint16_t length_counter = 0;
		uint32_t period_timer = 0;
		uint8_t duty_position = 0;
		envelope_state envelope;
	};

	struct sweep_state {
		uint16_t shadow_period = 0;
		uint8_t timer = 0;
		bool enabled = false;
		bool subtraction_used = false;
	};

	struct wave_channel_state {
		bool enabled = false;
		uint16_t length_counter = 0;
		uint32_t period_timer = 0;
		uint8_t sample_position = 0;
	};

	struct noise_channel_state {
		bool enabled = false;
		uint16_t length_counter = 0;
		uint32_t period_timer = 0;
		uint16_t lfsr = 0x7FFF;
		envelope_state envelope;
	};

	std::shared_ptr<shared::interrupt> interrupts;

	SpuRegisterFile registers;
	std::vector<stereo_sample> sample_buffer;
	pulse_channel_state channel1;
	pulse_channel_state channel2;
	sweep_state channel1_sweep;
	wave_channel_state channel3;
	noise_channel_state channel4;

	bool powered_on = false;
	uint32_t frame_sequencer_cycles = 0;
	uint8_t frame_sequencer_step = 0;
	uint32_t sample_clock_accumulator = 0;
	float left_filter_capacitor = 0.0f;
	float right_filter_capacitor = 0.0f;

	void reset_audio_registers();
	void power_off();
	void tick_frame_sequencer();
	void tick_channels();
	void tick_length_counters();
	void tick_envelope(envelope_state& envelope);
	void tick_sweep();
	void tick_pulse(pulse_channel_state& channel, uint16_t period_value);
	void tick_wave();
	void tick_noise();

	void trigger_channel1();
	void trigger_channel2();
	void trigger_channel3();
	void trigger_channel4();
	void trigger_envelope(envelope_state& envelope, uint8_t nrx2);

	[[nodiscard]] uint16_t channel1_period() const;
	[[nodiscard]] uint16_t channel2_period() const;
	[[nodiscard]] uint16_t channel3_period() const;
	[[nodiscard]] uint32_t channel4_timer_period() const;
	uint16_t calculate_sweep_period(bool update);
	[[nodiscard]] uint8_t pulse_output(
		const pulse_channel_state& channel,
		uint8_t duty) const;
	[[nodiscard]] uint8_t wave_output() const;
	[[nodiscard]] uint8_t noise_output() const;
	[[nodiscard]] uint8_t channel_status() const;
	[[nodiscard]] static bool envelope_dac_enabled(uint8_t nrx2);
	[[nodiscard]] float high_pass(float input, float& capacitor) const;
	void mix_sample();
};

#endif //SPU_H
