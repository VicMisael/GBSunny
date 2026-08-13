//
// Created by visael on 14/03/25.
//

#ifndef SPU_H
#define SPU_H

#include <array>
#include <cstdint>
#include <memory>
#include <shared/interrupt.h>
#include <vector>

// Sound processing unit.
//
// This is intentionally scaffold-only: the MMU can read/write audio registers,
// gb/main can drain samples into raylib, and the real channel generation can be
// filled in step by step.
class spu {
public:
	struct stereo_sample {
		float left;
		float right;
	};

	explicit spu(std::shared_ptr<shared::interrupt> interrupts);

	uint8_t read(uint16_t addr);
	uint8_t read_wave(uint16_t addr);
	void write(uint16_t addr, uint8_t data);
	void write_wave(uint16_t addr, uint8_t data);

	void step(uint32_t cycles);
	std::vector<stereo_sample> consume_samples();

private:
	static constexpr uint16_t AudioRegisterStart = 0xFF10;
	static constexpr uint16_t WaveRamStart = 0xFF30;

	std::shared_ptr<shared::interrupt> interrupts;

	std::array<uint8_t, 0x17> registers{};
	std::array<uint8_t, 16> wave_ram{};
	std::vector<stereo_sample> sample_buffer;

	bool powered_on = false;
	uint32_t frame_sequencer_cycles = 0;
	uint8_t frame_sequencer_step = 0;
	double sample_cycles = 0.0;

	void tick();
	void reset_audio_registers();
	void tick_frame_sequencer();
	void tick_channels();
	void mix_sample();

	[[nodiscard]] uint8_t register_index(uint16_t addr) const;
	[[nodiscard]] uint8_t read_mask(uint16_t addr) const;
};

#endif //SPU_H
