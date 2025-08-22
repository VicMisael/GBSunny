#pragma once
#include <cstdint>
#include <optional>


namespace ppu_fifo_types {

	struct fifo_element {
		uint8_t color;
		bool palette;
		uint8_t priority;
		bool bg_priority;
	};

	enum fifo_state {
		GET_TILE,
		GET_TILE_DATA_LOW,
		GET_TILE_DATA_HIGH,
		SLEEP,
		PUSH
	};


	struct OAM_priority_queue_element {
		ppu_types::OAM_Sprite sprite;
		uint16_t oam_index;
	};


	


};