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
#include <iomanip>
#include <sstream>

#include "utils/utils.h"

namespace {

auto cartridge_type_error(const CartridgeInfo& info) -> std::runtime_error {
    std::ostringstream stream;
    stream << "Unsupported cartridge type 0x"
           << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(info.type_code);
    return std::runtime_error(stream.str());
}

} // namespace

std::shared_ptr<Cartridge> instance_cartridge(const std::vector<uint8_t> &rom_data) noexcept(false) {
    std::unique_ptr<CartridgeInfo> info = get_info(rom_data);

    switch (info->type) {
        case CartridgeType::ROMOnly:
            return std::make_shared<NoMBC>(rom_data, std::move(info));
        case CartridgeType::MBC1:
            return std::make_shared<MBC1>(rom_data, std::move(info));
        case CartridgeType::MBC2:
            return std::make_shared<MBC2>(rom_data, std::move(info));
        case CartridgeType::MBC3:
            return std::make_shared<MBC3>(rom_data, std::move(info));
        case CartridgeType::MBC4:
            throw cartridge_type_error(*info);
        case CartridgeType::MBC5:
            return std::make_shared<MBC5>(rom_data, std::move(info));
        case CartridgeType::Unknown:
            throw cartridge_type_error(*info);
        default:
            break;
        //            fatal_error("Unknown cartridge type");
    }
    return nullptr;
}


Cartridge::Cartridge(std::unique_ptr<CartridgeInfo> in_cartridge_info)
    : cartridge_info(std::move(in_cartridge_info)) {
}

void Cartridge::set_rom0_bank_update_callback(std::function<void()> callback) {
    rom0_bank_update_callback = std::move(callback);
}

void Cartridge::set_romx_bank_update_callback(std::function<void()> callback) {
    romx_bank_update_callback = std::move(callback);
}

void Cartridge::notify_rom0_bank_update() const {
    if (rom0_bank_update_callback) {
        rom0_bank_update_callback();
    }
}

void Cartridge::notify_romx_bank_update() const {
    if (romx_bank_update_callback) {
        romx_bank_update_callback();
    }
}

uint8_t Cartridge::read_rom_byte(const std::vector<uint8_t>& rom, uint16_t bank, uint16_t address) {
    if (rom.empty()) {
        return 0xFF;
    }

    const uint32_t offset = static_cast<uint32_t>(bank) * 0x4000 + (address & 0x3FFF);
    if (offset >= rom.size()) {
        return 0xFF;
    }
    return rom[offset];
}

uint8_t Cartridge::read_ram_byte(const std::vector<uint8_t>& ram, uint16_t bank, uint16_t address) {
    if (ram.empty()) {
        return 0xFF;
    }

    const uint32_t offset = static_cast<uint32_t>(bank) * 0x2000 + (address - 0xA000);
    if (offset >= ram.size()) {
        return 0xFF;
    }
    return ram[offset];
}

void Cartridge::write_ram_byte(std::vector<uint8_t>& ram, uint16_t bank, uint16_t address, uint8_t value) {
    if (ram.empty()) {
        return;
    }

    const uint32_t offset = static_cast<uint32_t>(bank) * 0x2000 + (address - 0xA000);
    if (offset < ram.size()) {
        ram[offset] = value;
    }
}

uint16_t Cartridge::rom_bank_count(const std::vector<uint8_t>& rom) const {
    const auto banks = static_cast<uint16_t>(rom.size() / 0x4000);
    return banks == 0 ? 1 : banks;
}

uint8_t Cartridge::ram_bank_count(const std::vector<uint8_t>& ram) const {
    const auto banks = static_cast<uint8_t>((ram.size() + 0x1FFF) / 0x2000);
    return banks == 0 ? 1 : banks;
}

std::shared_ptr<Cartridge> Cartridge::get_cartridge(const std::string &path) {
    const auto file = utils::read_file(path);
    auto cartridge = instance_cartridge(file);
    return cartridge;
}

std::shared_ptr<Cartridge> Cartridge::get_cartridge(const std::string &path,
                                                    EmulatorEventAggregator& event_bus) {
    auto cartridge = get_cartridge(path);
    event_bus.send(CartridgeLoadedEvent{
        .rom_path = path,
        .rom_name = cartridge->cartridge_info->title,
        .rom_type = describe(cartridge->cartridge_info->type),
    });
    return cartridge;
}


NoMBC::NoMBC(std::vector<uint8_t> rom_data,
             std::unique_ptr<CartridgeInfo> in_cartridge_info)
    : rom(std::move(rom_data)),Cartridge(std::move(in_cartridge_info)) {
    if (cartridge_info->has_ram) {
        ram.resize(get_actual_ram_size(cartridge_info->ram_size));
    }
}

void NoMBC::write_sram(uint16_t addr,uint8_t value)
{
    if (addr < 0xA000 || addr > 0xBFFF) {
        return;
    }
    write_ram_byte(ram, 0, addr, value);
}

void NoMBC::write(const uint16_t &uint16_t, uint8_t value) {
}

auto NoMBC::read(const uint16_t &addr) const -> uint8_t {
    if (addr >= rom.size()) {
        return 0xFF;
    }
    return rom[addr];
}

const uint8_t* NoMBC::rom0_data() const {
    return rom.empty() ? nullptr : rom.data();
}

const uint8_t* NoMBC::romx_data() const {
    return rom.size() > 0x4000 ? rom.data() + 0x4000 : nullptr;
}

uint8_t NoMBC::read_sram(uint16_t addr) const
{
    if (addr < 0xA000 || addr > 0xBFFF) {
        return 0xFF;
    }
    return read_ram_byte(ram, 0, addr);
}
