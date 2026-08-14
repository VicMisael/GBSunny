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

	[[nodiscard]] uint8_t read(uint16_t addr) const;
	[[nodiscard]] uint8_t read_wave(uint16_t addr) const;
	void write(uint16_t addr, uint8_t data);
	void write_wave(uint16_t addr, uint8_t data);

	void step(uint32_t cycles);
	void tick();
	std::vector<stereo_sample> consume_samples();

private:
	std::shared_ptr<shared::interrupt> interrupts;

	SpuRegisterFile registers;
	std::vector<stereo_sample> sample_buffer;

	bool powered_on = false;
	uint32_t frame_sequencer_cycles = 0;
	uint8_t frame_sequencer_step = 0;
	double sample_cycles = 0.0;


	void reset_audio_registers();
	void tick_frame_sequencer();
	void tick_channels();
	void mix_sample();
};

#endif //SPU_H
