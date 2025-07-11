//Copyright (c) 2015-2021 Jonathan Gilchrist
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions are met:
//
//* Redistributions of source code must retain the above copyright notice, this
//  list of conditions and the following disclaimer.
//
//* Redistributions in binary form must reproduce the above copyright notice,
//  this list of conditions and the following disclaimer in the documentation
//  and/or other materials provided with the distribution.
//
//* Neither the name of gbemu nor the names of its
//  contributors may be used to endorse or promote products derived from
//  this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#include "cartridge.h"

#include <utility>
#include <stdexcept>

#include "utils/utils.h"


std::shared_ptr<Cartridge> instance_cartridge(const std::vector<uint8_t> &rom_data) noexcept(false) {
    std::unique_ptr<CartridgeInfo> info = get_info(rom_data);

    switch (info->type) {
        case CartridgeType::ROMOnly:
            return std::make_shared<NoMBC>(rom_data, std::move(info));
        case CartridgeType::MBC1:
            return std::make_shared<MBC1>(rom_data, std::move(info));
        case CartridgeType::MBC2:
            throw std::runtime_error("MBC2 not implemented");
        case CartridgeType::MBC3:
            throw std::runtime_error("MBC3 not implemented");
        case CartridgeType::MBC4:
            throw std::runtime_error("MBC4 not implemented");
        case CartridgeType::MBC5:
            throw std::runtime_error("MBC5 not implemented");
        case CartridgeType::Unknown:
            throw std::runtime_error("Unknown Cartridge type");
        default:
            break;
        //            fatal_error("Unknown cartridge type");
    }
    return nullptr;
}


Cartridge::Cartridge(std::vector<uint8_t> rom_data,std::unique_ptr<CartridgeInfo> in_cartridge_info)
    : rom(std::move(rom_data)), cartridge_info(std::move(in_cartridge_info)) {
    auto ram_size_for_cartridge = get_actual_ram_size(cartridge_info->ram_size);
    ram = std::vector<uint8_t>(ram_size_for_cartridge, 0);
}

std::shared_ptr<Cartridge> Cartridge::get_cartridge(const std::string &path) {
    std::vector<uint8_t> ram = {};
    const auto file = utils::read_file(path);
    auto cartridge = instance_cartridge(file);
    return cartridge;
}


NoMBC::NoMBC(std::vector<uint8_t> rom_data,
             std::unique_ptr<CartridgeInfo> in_cartridge_info)
    : Cartridge(std::move(rom_data), std::move(in_cartridge_info)) {
}

void NoMBC::write_sram(uint16_t addr,uint8_t value)
{

}

void NoMBC::write(const uint16_t &uint16_t, uint8_t value) {
}

auto NoMBC::read(const uint16_t &addr) const -> uint8_t {
    /* TODO: check this uint16_t is in sensible bounds */
    return rom[addr];
}

uint8_t NoMBC::read_sram(uint16_t addr) const
{
    return 0;
}



MBC1::MBC1(std::vector<uint8_t> rom_data, std::unique_ptr<CartridgeInfo> in_cartridge_info)
    : Cartridge(std::move(rom_data), std::move(in_cartridge_info)) {
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
        if (rom_banking_mode) {
            current_rom_bank = (current_rom_bank & 0x1F) | ((value & 0x03) << 5);
            current_rom_bank %= rom.size() / 0x4000;
            if (current_rom_bank == 0) current_rom_bank = 1;
        }
        else {
            current_ram_bank = value & 0x03;
        }
    }
    else if (address <= 0x7FFF) {
        rom_banking_mode = (value & 0x01) == 1;
    }
}

uint8_t MBC1::read(const uint16_t& address) const {
    if (address <= 0x3FFF) {
        return rom[address];
    }
    else if (address <= 0x7FFF) {
        uint32_t mapped_addr = (current_rom_bank * 0x4000) + (address - 0x4000);
        if (mapped_addr < rom.size()) return rom[mapped_addr];
    }
    return 0xFF;
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
    uint32_t mapped_addr = (current_ram_bank * 0x2000) + (addr - 0xA000);
    if (mapped_addr < ram.size()) {
        return ram[mapped_addr];
    }
    return 0xFF;
}