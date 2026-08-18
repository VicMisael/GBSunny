#include "../cartridge.h"
#include "utils/utils.h"

MBC5::MBC5(std::vector<uint8_t> rom_data, std::unique_ptr<CartridgeInfo> in_cartridge_info)
	: rom(std::move(rom_data)), Cartridge(std::move(in_cartridge_info)) {
	ram.resize(get_actual_ram_size(cartridge_info->ram_size));
}

uint8_t MBC5::read(const uint16_t& address) const {
	if (utils::in_range(0, 0x3FFF, address)) {
		return read_rom_byte(rom, 0, address);
	}

	return read_rom_byte(rom, current_rom_bank % rom_bank_count(rom), address);
}

void MBC5::write(const uint16_t& address, uint8_t value) {
	if (address <= 0x1FFF) {
		ram_enabled = (value & 0x0F) == 0x0A;
	}
	else if (address <= 0x2FFF) {
		current_rom_bank = (current_rom_bank & 0x100) | value;
		current_rom_bank %= rom_bank_count(rom);
	}
	else if (address <= 0x3FFF) {
		current_rom_bank = (current_rom_bank & 0x0FF) | ((value & 0x01) << 8);
		current_rom_bank %= rom_bank_count(rom);
	}
	else if (address <= 0x5FFF) {
		if (cartridge_info->has_rumble) {
			rumble_enabled = (value & 0x08) != 0;
			current_ram_bank = value & 0x07;
		}
		else {
			current_ram_bank = value & 0x0F;
		}
		current_ram_bank %= ram_bank_count(ram);
	}
}

uint8_t MBC5::read_sram(uint16_t addr) const {
	if (!ram_enabled || addr < 0xA000 || addr > 0xBFFF) {
		return 0xFF;
	}

	return read_ram_byte(ram, current_ram_bank, addr);
}

void MBC5::write_sram(uint16_t addr, uint8_t value) {
	if (!ram_enabled || addr < 0xA000 || addr > 0xBFFF) {
		return;
	}

	write_ram_byte(ram, current_ram_bank, addr, value);
}
