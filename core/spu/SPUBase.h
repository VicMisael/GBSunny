#ifndef SPU_BASE_H
#define SPU_BASE_H

#include <cstdint>
#include <vector>

class SPUBase {
public:
	struct stereo_sample {
		float left;
		float right;
	};

	virtual ~SPUBase() = default;

	[[nodiscard]] virtual uint8_t read(uint16_t addr) const = 0;
	[[nodiscard]] virtual uint8_t read_wave(uint16_t addr) const = 0;
	virtual void write(uint16_t addr, uint8_t data) = 0;
	virtual void write_wave(uint16_t addr, uint8_t data) = 0;
	virtual void step(uint32_t cycles) = 0;
	virtual void tick() = 0;
	virtual std::vector<stereo_sample> consume_samples() = 0;
};

#endif //SPU_BASE_H
