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
            throw std::runtime_error("MBC1 not implemented");
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
    return instance_cartridge(utils::read_file(path));

}


NoMBC::NoMBC(std::vector<uint8_t> rom_data,
             std::unique_ptr<CartridgeInfo> in_cartridge_info)
    : Cartridge(std::move(rom_data), std::move(in_cartridge_info)) {
}

void NoMBC::write_sram(uint16_t addr)
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
