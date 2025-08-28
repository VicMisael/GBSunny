//
// Created by Misael on 12/07/2025.
//

#include "../cartridge.h"


MBC1::MBC1(std::vector<uint8_t> rom_data, std::unique_ptr<CartridgeInfo> in_cartridge_info)
    : rom(std::move(rom_data)),Cartridge( std::move(in_cartridge_info)) {
    const uint32_t ram_size_bytes = get_actual_ram_size(cartridge_info->ram_size);
    ram.resize(ram_size_bytes);
    current_rom_bank = 1;
}

void MBC1::write(const uint16_t& address, uint8_t value) {
    if (address <= 0x1FFF) {
        ram_enabled = (value & 0x0F) == 0x0A;
    }
    else if (address <= 0x3FFF) {
        uint8_t lower5 = value & 0x1F;
        if (lower5 == 0) lower5 = 1;
        current_rom_bank = (current_rom_bank & 0xE0) | lower5;
        current_rom_bank %= rom.size() / 0x4000;
        if (current_rom_bank == 0) current_rom_bank = 1;
    }
    else if (address <= 0x5FFF) {
        if (!rom_banking_mode) {
            // ROM banking mode: use upper 2 bits for ROM bank
            current_rom_bank = (current_rom_bank & 0x1F) | ((value & 0x03) << 5);
            uint32_t max_banks = rom.size() / 0x4000;
            if (current_rom_bank >= max_banks) current_rom_bank %= max_banks;
            if (current_rom_bank == 0) current_rom_bank = 1;
        }
        else {
            // RAM banking mode
            current_ram_bank = value & 0x03;
        }
    }
    else if (address <= 0x7FFF) {
        rom_banking_mode = (value & 0x01) == 1;
    }
}

uint8_t MBC1::read(const uint16_t& address) const {
    if (address < 0x4000) {
        return rom[address];
    }

    const auto bank = std::span<const uint8_t>(&rom[current_rom_bank * 0x4000], 0x4000);
    return bank[address % 0x4000];
}

void MBC1::write_sram(uint16_t addr, uint8_t value) {
    if (!ram_enabled || addr < 0xA000 || addr > 0xBFFF) return;
    uint32_t mapped_addr = (current_ram_bank * 0x2000) + (addr - 0xA000);
    if (mapped_addr < ram.size()) {
        ram[mapped_addr] = value;
    }
}

uint8_t MBC1::read_sram(uint16_t addr) const {
    if (!ram_enabled || addr < 0xA000 || addr > 0xBFFF) return 0xFF;

    const auto bank = std::span<const uint8_t>(&ram[current_ram_bank * 0x2000], 0x2000);
    return bank[addr % 0x2000];

}