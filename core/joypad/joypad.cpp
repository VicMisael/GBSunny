#include "joypad.h"

#include <utility>

Joypad::Joypad(std::shared_ptr<shared::interrupt> interrupt_controller)
	: interrupts(std::move(interrupt_controller)) {
}

void Joypad::reset() {
	selected = {};
	pressed_buttons = 0;
}

uint8_t Joypad::read() const {
	uint8_t lines = ButtonLines;

	if (selected.directions) {
		lines &= static_cast<uint8_t>(~pressed_buttons) & ButtonLines;
	}

	if (selected.actions) {
		lines &= static_cast<uint8_t>(~(pressed_buttons >> 4)) & ButtonLines;
	}

	uint8_t result = 0xC0 | lines;
	if (!selected.directions) {
		result |= 0x10;
	}
	if (!selected.actions) {
		result |= 0x20;
	}

	return result;
}

void Joypad::write(uint8_t data) {
	const uint8_t previous_lines = read() & ButtonLines;
	selected.directions = (data & 0x10) == 0;
	selected.actions = (data & 0x20) == 0;
	request_interrupt_on_falling_edge(previous_lines);
}

void Joypad::set_button(JoypadButton button, bool pressed) {
	const uint8_t previous_lines = read() & ButtonLines;
	const uint8_t mask = uint8_t{1} << static_cast<uint8_t>(button);

	if (pressed) {
		pressed_buttons |= mask;
	}
	else {
		pressed_buttons &= static_cast<uint8_t>(~mask);
	}

	request_interrupt_on_falling_edge(previous_lines);
}

void Joypad::request_interrupt_on_falling_edge(uint8_t previous_lines) const {
	const uint8_t current_lines = read() & ButtonLines;
	if ((previous_lines & static_cast<uint8_t>(~current_lines) & ButtonLines) != 0) {
		interrupts->requested.joypad = true;
	}
}
