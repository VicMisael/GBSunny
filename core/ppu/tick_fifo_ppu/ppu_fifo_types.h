#pragma once
#include <cstdint>


namespace ppu_fifo_types {

	struct fifo_element {
		uint8_t color;
		uint8_t palette;
		uint8_t priority;
		bool bg_priority;
	};

	enum bg_fifo_state {
		GET_TILE,
		GET_TILE_DATA_LOW,
		GET_TILE_DATA_HIGH,
		SLEEP,
		PUSH
	} ;

};