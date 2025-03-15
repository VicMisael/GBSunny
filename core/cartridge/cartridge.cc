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
