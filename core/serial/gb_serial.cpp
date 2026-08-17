#include "gb_serial.h"

#include <ostream>

namespace {
	constexpr uint16_t SERIAL_DATA_REGISTER = 0xFF01;
	constexpr uint16_t SERIAL_CONTROL_REGISTER = 0xFF02;
	constexpr uint8_t SERIAL_TRANSFER_START = 0x80;
}

serial::ConsoleGBSerial::ConsoleGBSerial(std::ostream& output) : output(output) {
}

void serial::ConsoleGBSerial::reset() {
	serial_data = 0x00;
	serial_control = 0x00;
	header_written = false;
}

uint8_t serial::ConsoleGBSerial::read(const uint16_t addr) const {
	switch (addr) {
	case SERIAL_DATA_REGISTER:
		return serial_data;
	case SERIAL_CONTROL_REGISTER:
		return serial_control;
	default:
		return 0xFF;
	}
}

void serial::ConsoleGBSerial::write(const uint16_t addr, const uint8_t data) {
	switch (addr) {
	case SERIAL_DATA_REGISTER:
		serial_data = data;
		return;
	case SERIAL_CONTROL_REGISTER:
		serial_control = data;
		if ((serial_control & SERIAL_TRANSFER_START) != 0) {
			start_transfer();
		}
		return;
	default:
		return;
	}
}

void serial::ConsoleGBSerial::start_transfer() {
	if (!header_written) {
		output << "Serial out: ";
		header_written = true;
	}

	output << static_cast<char>(serial_data);
	output.flush();

	serial_control &= ~SERIAL_TRANSFER_START;
	serial_data = 0xFF;
}
