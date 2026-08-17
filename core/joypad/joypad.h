#ifndef JOYPAD_H
#define JOYPAD_H

#include <cstdint>
#include <memory>

#include "shared/interrupt.h"

enum class JoypadButton : uint8_t {
	Right,
	Left,
	Up,
	Down,
	A,
	B,
	Select,
	Start
};

class Joypad {
public:
	explicit Joypad(std::shared_ptr<shared::interrupt> interrupt_controller);

	void reset();
	[[nodiscard]] uint8_t read() const;
	void write(uint8_t data);
	void set_button(JoypadButton button, bool pressed);

private:
	static constexpr uint8_t ButtonLines = 0x0F;

	struct Selection {
		bool directions = true;
		bool actions = true;
	};

	Selection selected;
	uint8_t pressed_buttons = 0;
	std::shared_ptr<shared::interrupt> interrupts;

	void request_interrupt_on_falling_edge(uint8_t previous_lines) const;
};

#endif // JOYPAD_H
