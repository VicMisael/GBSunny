#ifndef GB_SERIAL_H
#define GB_SERIAL_H

#include <cstdint>
#include <iosfwd>

namespace serial {
	class GBSerial {
	public:
		virtual ~GBSerial() = default;

		virtual void reset() = 0;
		[[nodiscard]] virtual uint8_t read(uint16_t addr) const = 0;
		virtual void write(uint16_t addr, uint8_t data) = 0;
	};

	class NullGBSerial final : public GBSerial {
	public:
		void reset() override {}
		[[nodiscard]] uint8_t read(uint16_t) const override { return 0xFF; }
		void write(uint16_t, uint8_t) override {}
	};

	class ConsoleGBSerial final : public GBSerial {
		uint8_t serial_data = 0x00;    // 0xFF01 (SB)
		uint8_t serial_control = 0x00; // 0xFF02 (SC)
		std::ostream& output;
		bool header_written = false;

		void start_transfer();

	public:
		explicit ConsoleGBSerial(std::ostream& output);

		void reset() override;
		[[nodiscard]] uint8_t read(uint16_t addr) const override;
		void write(uint16_t addr, uint8_t data) override;
	};
}

#endif // GB_SERIAL_H
