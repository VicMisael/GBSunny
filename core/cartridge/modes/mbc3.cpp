
#include "../cartridge.h"
#include "utils/utils.h"

MBC3::MBC3(std::vector<uint8_t> rom_data, std::unique_ptr<CartridgeInfo> in_cartridge_info)
	:rom(std::move(rom_data)), Cartridge(std::move(in_cartridge_info)) {
	ram.resize(get_actual_ram_size(cartridge_info->ram_size));
}

uint8_t MBC3::read(const uint16_t& address) const
{
	if (utils::in_range(0, 0x3fff, address)) {
		return read_rom_byte(rom, 0, address);
	}

	return read_rom_byte(rom, current_rom_bank % rom_bank_count(rom), address);
}

const uint8_t* MBC3::rom0_data() const {
	return rom.empty() ? nullptr : rom.data();
}

const uint8_t* MBC3::romx_data() const {
	const auto offset = static_cast<uint32_t>(current_rom_bank) * 0x4000;
	if (rom.empty() || offset >= rom.size()) {
		return nullptr;
	}

	return rom.data() + offset;
}

void MBC3::write(const uint16_t& address, uint8_t value)
{
	if (address <= 0x1FFF) {
		ram_enabled = (value & 0x0F) == 0x0A;
	}
	else if (address <= 0x3FFF) {
		auto new_current_rom_bank = static_cast<uint8_t>(value & 0x7F);
		if (new_current_rom_bank == 0) new_current_rom_bank = 1;
		new_current_rom_bank %= rom_bank_count(rom);
		if (new_current_rom_bank == 0) new_current_rom_bank = 1;
		if (current_rom_bank != new_current_rom_bank) {
			current_rom_bank = new_current_rom_bank;
			notify_romx_bank_update();
		}
	}
	else if (address <= 0x5FFF) {
		selected_ram_or_rtc = value;
	}
	else if (address <= 0x7FFF) {
		if (value == 0) {
			latch_armed = true;
		}
		else if (value == 1 && latch_armed) {
			latched_rtc = rtc;
			latch_armed = false;
		}
	}
}

uint8_t MBC3::read_sram(uint16_t addr) const
{
	if (!ram_enabled || addr < 0xA000 || addr > 0xBFFF) {
		return 0xFF;
	}

	if (rtc_register_selected()) {
		return read_rtc();
	}

	const uint8_t max_ram_bank = ram.size() > 0x8000 ? 0x07 : 0x03;
	if (selected_ram_or_rtc <= max_ram_bank) {
		const uint8_t bank_mask = ram.size() > 0x8000 ? 0x07 : 0x03;
		return read_ram_byte(ram, selected_ram_or_rtc & bank_mask, addr);
	}

	return 0xff;
}

void MBC3::write_sram(uint16_t addr, uint8_t value)
{
	if (!ram_enabled || addr < 0xA000 || addr > 0xBFFF) {
		return;
	}

	if (rtc_register_selected()) {
		write_rtc(value);
		return;
	}

	const uint8_t max_ram_bank = ram.size() > 0x8000 ? 0x07 : 0x03;
	if (selected_ram_or_rtc <= max_ram_bank) {
		const uint8_t bank_mask = ram.size() > 0x8000 ? 0x07 : 0x03;
		write_ram_byte(ram, selected_ram_or_rtc & bank_mask, addr, value);
	}

}

bool MBC3::rtc_register_selected() const {
	return cartridge_info->has_timer && selected_ram_or_rtc >= 0x08 && selected_ram_or_rtc <= 0x0C;
}

uint8_t MBC3::read_rtc() const {
	switch (selected_ram_or_rtc) {
		case 0x08:
			return latched_rtc.seconds;
		case 0x09:
			return latched_rtc.minutes;
		case 0x0A:
			return latched_rtc.hours;
		case 0x0B:
			return latched_rtc.day_low;
		case 0x0C:
			return latched_rtc.day_high;
		default:
			return 0xFF;
	}
}

void MBC3::write_rtc(uint8_t value) {
	switch (selected_ram_or_rtc) {
		case 0x08:
			rtc.seconds = value % 60;
			break;
		case 0x09:
			rtc.minutes = value % 60;
			break;
		case 0x0A:
			rtc.hours = value % 24;
			break;
		case 0x0B:
			rtc.day_low = value;
			break;
		case 0x0C:
			rtc.day_high = value & 0xC1;
			break;
		default:
			break;
	}
}
