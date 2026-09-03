
#include "../cartridge.h"
#include "utils/utils.h"

MBC2::MBC2(std::vector<uint8_t> rom_data, std::unique_ptr<CartridgeInfo> in_cartridge_info)
	:rom(std::move(rom_data)), Cartridge(std::move(in_cartridge_info)) {

}

uint8_t MBC2::read(const uint16_t& address) const
{
	if (utils::in_range(0, 0x3fff, address)) {
		return read_rom_byte(rom, 0, address);
	}

	return read_rom_byte(rom, current_rom_bank % rom_bank_count(rom), address);
}

const uint8_t* MBC2::rom0_data() const {
	return rom.empty() ? nullptr : rom.data();
}

void MBC2::write(const uint16_t& address, uint8_t value)
{
	if (address > 0x3FFF) {
		return;
	}

	const bool _8thbit = address & 0x100;
	if (_8thbit) {
		//8thbit set
		const auto bank = value & 0xf;
		current_rom_bank = bank == 0 ? 1 : bank;
		current_rom_bank %= rom_bank_count(rom);
		if (current_rom_bank == 0) current_rom_bank = 1;
	}
	else {
		//8thbit unset
		ram_enabled = (value & 0xf) == 0xA;
	}
}

uint8_t MBC2::read_sram(uint16_t addr) const
{
	if (ram_enabled && addr >= 0xA000 && addr <= 0xBFFF)
		return 0xF0 | (ram[addr & 0x1FF] & 0x0F);
	return 0xff;
}

void MBC2::write_sram(uint16_t addr, uint8_t value)
{
	if (ram_enabled && addr >= 0xA000 && addr <= 0xBFFF)
		ram[addr & 0x1FF] = value & 0xf;

}
