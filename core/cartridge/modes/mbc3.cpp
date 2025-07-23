
#include "../cartridge.h"
#include "utils/utils.h"

MBC3::MBC3(std::vector<uint8_t> rom_data, std::unique_ptr<CartridgeInfo> in_cartridge_info)
	:rom(std::move(rom_data)), Cartridge(std::move(in_cartridge_info)) {

}

uint8_t MBC3::read(const uint16_t& address) const
{
	if (utils::in_range(0, 0x3fff, address)) {
		return rom[address];
	}

	const auto bank = std::span<const uint8_t>(&rom[current_bank * 0x4000], 0x4000);
	return bank[address % 0x4000];
}

void MBC3::write(const uint16_t& address, uint8_t value)
{
	const bool _8thbit = address & 0x100;
	if (_8thbit) {
		//8thbit set
		const auto bank = value & 0xf;
		current_bank = bank == 0 ? 1 : bank;
	}
	else {
		//8thbit unset
		ram_enabled = (value & 0xf) == 0xA;
	}
}

uint8_t MBC3::read_sram(uint16_t addr) const
{
	if (ram_enabled)
		return ram[addr & 0x1FF];
	return 0xff;
}

void MBC3::write_sram(uint16_t addr, uint8_t value)
{
	if (ram_enabled)
		ram[addr & 0x1FF] = value & 0xf;

}