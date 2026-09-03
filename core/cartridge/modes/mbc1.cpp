//
// Created by Misael on 12/07/2025.
//

#include "../cartridge.h"


MBC1::MBC1(std::vector<uint8_t> rom_data, std::unique_ptr<CartridgeInfo> in_cartridge_info)
    : Cartridge(std::move(in_cartridge_info)), rom(std::move(rom_data)) {
    const uint32_t ram_size_bytes = get_actual_ram_size(cartridge_info->ram_size);
    ram.resize(ram_size_bytes);
    lower_rom_bank_bits = 1;
}

void MBC1::write(const uint16_t& address, uint8_t value) {
    if (address <= 0x1FFF) {
        ram_enabled = (value & 0x0F) == 0x0A;
    }
    else if (address <= 0x3FFF) {
        uint8_t new_lower_rom_bank_bits = value & 0x1F;
        if (new_lower_rom_bank_bits == 0) new_lower_rom_bank_bits = 1;
        if (lower_rom_bank_bits != new_lower_rom_bank_bits) {
            lower_rom_bank_bits = new_lower_rom_bank_bits;
            notify_romx_bank_update();
        }
    }
    else if (address <= 0x5FFF) {
        const uint8_t new_upper_bank_bits = value & 0x03;
        if (upper_bank_bits != new_upper_bank_bits) {
            upper_bank_bits = new_upper_bank_bits;
            notify_rom0_bank_update();
            notify_romx_bank_update();
        }
    }
    else if (address <= 0x7FFF) {
        const bool new_advanced_banking_mode = (value & 0x01) == 1;
        if (advanced_banking_mode != new_advanced_banking_mode) {
            advanced_banking_mode = new_advanced_banking_mode;
            notify_rom0_bank_update();
            notify_romx_bank_update();
        }
    }
}

uint8_t MBC1::read(const uint16_t& address) const {
    if (address < 0x4000) {
        return read_rom_byte(rom, fixed_rom_bank(), address);
    }

    return read_rom_byte(rom, switchable_rom_bank(), address);
}

const uint8_t* MBC1::rom0_data() const {
    const auto offset = static_cast<uint32_t>(fixed_rom_bank()) * 0x4000;
    if (rom.empty() || offset >= rom.size()) {
        return nullptr;
    }

    return rom.data() + offset;
}

const uint8_t* MBC1::romx_data() const {
    const auto offset = static_cast<uint32_t>(switchable_rom_bank()) * 0x4000;
    if (rom.empty() || offset >= rom.size()) {
        return nullptr;
    }

    return rom.data() + offset;
}

void MBC1::write_sram(uint16_t addr, uint8_t value) {
    if (!ram_enabled || addr < 0xA000 || addr > 0xBFFF) return;
    write_ram_byte(ram, selected_ram_bank(), addr, value);
}

uint8_t MBC1::read_sram(uint16_t addr) const {
    if (!ram_enabled || addr < 0xA000 || addr > 0xBFFF) return 0xFF;

    return read_ram_byte(ram, selected_ram_bank(), addr);

}

uint16_t MBC1::switchable_rom_bank() const {
    const auto banks = rom_bank_count(rom);
    uint16_t bank = lower_rom_bank_bits | (static_cast<uint16_t>(upper_bank_bits) << 5);
    bank %= banks;
    if ((bank & 0x1F) == 0) {
        bank = (bank + 1) % banks;
    }
    return bank;
}

uint16_t MBC1::fixed_rom_bank() const {
    if (!advanced_banking_mode) {
        return 0;
    }

    return (static_cast<uint16_t>(upper_bank_bits) << 5) % rom_bank_count(rom);
}

uint8_t MBC1::selected_ram_bank() const {
    if (!advanced_banking_mode) {
        return 0;
    }

    return upper_bank_bits % ram_bank_count(ram);
}
