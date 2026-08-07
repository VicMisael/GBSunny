//
// Created by visael on 14/03/25.
//

#ifndef SPU_H
#define SPU_H
#include <cstdint>
#include <shared/interrupt.h>
#include <memory>


class base_channel {
	uint8_t nrx1;

};

//Sound processing UNIT
class spu {
	std::shared_ptr<shared::interrupt> interrupts;
	void tick();
    union {
        uint8_t nr_50_data;
        struct {
            uint8_t right_volume : 3;  // bits 0-2
            bool vin_right : 1;  // bit 3
            uint8_t left_volume : 3;  // bits 4-6
            bool vin_left : 1;  // bit 7
        };
    } nr_50;

    union {
        uint8_t nr_51_data;
        struct {
            bool ch1_right : 1; // bit 0
            bool ch2_right : 1; // bit 1
            bool ch3_right : 1; // bit 2
            bool ch4_right : 1; // bit 3

            bool ch1_left : 1; // bit 4
            bool ch2_left : 1; // bit 5
            bool ch3_left : 1; // bit 6
            bool ch4_left : 1; // bit 7
        };
    } nr_51;

    union {
        uint8_t nr_52_data;
        struct {
            bool ch1_on : 1; // bit 0
            bool ch2_on : 1; // bit 1
            bool ch3_on : 1; // bit 2
            bool ch4_on : 1; // bit 3
            uint8_t unused : 3; // bits 4-6
            bool power_on : 1; // bit 7
        };
    } nr_52;
public:
	explicit spu(std::shared_ptr<shared::interrupt> interrupts) {
		this->interrupts = std::move(interrupts);
	}

	uint8_t read(uint16_t addr);
	uint8_t read_wave(uint16_t addr);
	void write(uint16_t addr, uint8_t data);
	void write_wave(uint16_t addr, uint8_t data);

	void step(uint32_t cycles);
};



#endif //SPU_H
