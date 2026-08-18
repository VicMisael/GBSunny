#pragma once
#include <string>
#include <variant>
#include "event_aggregator.h"

struct VBlankEvent {

};

struct HBlankEvent {
};

struct FrameCompleteEvent {

};


struct PPUFrameCompleteEvent{
};

struct InterruptSet {
};

struct CartridgeLoadedEvent {
	std::string rom_path;
	std::string rom_name;
	std::string rom_type;
};

using EmulatorEventAggregator = EventAggregator<VBlankEvent, HBlankEvent, FrameCompleteEvent, PPUFrameCompleteEvent, InterruptSet, CartridgeLoadedEvent>;