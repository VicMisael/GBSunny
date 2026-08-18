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


#pragma once

#include "cartridge_info.h"

#include <string>
#include <vector>
#include <memory>
#include <span>
#include <array>
#include <cstdint>

class Cartridge {
public:
    Cartridge(std::unique_ptr<CartridgeInfo> cartridge_info);

    virtual ~Cartridge() = default;

    [[nodiscard]] virtual uint8_t read(const uint16_t &address) const = 0;

    virtual void write(const uint16_t &address, uint8_t value) = 0;
    [[nodiscard]] virtual uint8_t read_sram(uint16_t addr) const = 0;
    virtual void write_sram(uint16_t addr,uint8_t value) = 0;

    static std::shared_ptr<Cartridge> get_cartridge(const std::string &path);
    std::unique_ptr<CartridgeInfo> cartridge_info;

protected:
    [[nodiscard]] static uint8_t read_rom_byte(const std::vector<uint8_t>& rom,
                                               uint16_t bank,
                                               uint16_t address);
    [[nodiscard]] static uint8_t read_ram_byte(const std::vector<uint8_t>& ram,
                                               uint16_t bank,
                                               uint16_t address);
    static void write_ram_byte(std::vector<uint8_t>& ram,
                               uint16_t bank,
                               uint16_t address,
                               uint8_t value);
    [[nodiscard]] uint16_t rom_bank_count(const std::vector<uint8_t>& rom) const;
    [[nodiscard]] uint8_t ram_bank_count(const std::vector<uint8_t>& ram) const;
};


class NoMBC : public Cartridge {
public:
    NoMBC(std::vector<uint8_t> rom_data,
          std::unique_ptr<CartridgeInfo> cartridge_info);

    [[nodiscard]] uint8_t read(const uint16_t &address) const override;

    [[nodiscard]] uint8_t read_sram(uint16_t addr) const override;

    void write_sram(uint16_t addr,uint8_t value) override;
    void write(const uint16_t &address, uint8_t value) override;
private:
    std::vector<uint8_t> rom;
    std::vector<uint8_t> ram;

};





class MBC1 : public Cartridge {
public:
    MBC1(std::vector<uint8_t> rom_data, std::unique_ptr<CartridgeInfo> in_cartridge_info);

    // Updated to match Cartridge base class signature exactly
    [[nodiscard]] uint8_t read(const uint16_t& address) const override;
    void write(const uint16_t& address, uint8_t value) override;

    [[nodiscard]] uint8_t read_sram(uint16_t addr) const override;
    void write_sram(uint16_t addr, uint8_t value) override;

private:
    std::vector<uint8_t> rom;
    std::vector<uint8_t> ram;

    uint8_t lower_rom_bank_bits = 1;
    uint8_t upper_bank_bits = 0;
    bool ram_enabled = false;
    bool advanced_banking_mode = false;

    [[nodiscard]] uint16_t switchable_rom_bank() const;
    [[nodiscard]] uint16_t fixed_rom_bank() const;
    [[nodiscard]] uint8_t selected_ram_bank() const;
};


class MBC2 : public Cartridge {
public:
    MBC2(std::vector<uint8_t> rom_data, std::unique_ptr<CartridgeInfo> in_cartridge_info);

    // Updated to match Cartridge base class signature exactly
    [[nodiscard]] uint8_t read(const uint16_t& address) const override;
    void write(const uint16_t& address, uint8_t value) override;

    [[nodiscard]] uint8_t read_sram(uint16_t addr) const override;
    void write_sram(uint16_t addr, uint8_t value) override;



private:
    std::vector<uint8_t> rom;
    std::array<uint8_t,512> ram = {};

    //State
    bool ram_enabled=false;
    uint8_t current_rom_bank=1;
};

class MBC3 : public Cartridge {
public:
    MBC3(std::vector<uint8_t> rom_data, std::unique_ptr<CartridgeInfo> in_cartridge_info);

    // Updated to match Cartridge base class signature exactly
    [[nodiscard]] uint8_t read(const uint16_t& address) const override;
    void write(const uint16_t& address, uint8_t value) override;

    [[nodiscard]] uint8_t read_sram(uint16_t addr) const override;
    void write_sram(uint16_t addr, uint8_t value) override;



private:
    std::vector<uint8_t> rom;
    std::vector<uint8_t> ram;

    struct RtcRegisters {
        uint8_t seconds = 0;
        uint8_t minutes = 0;
        uint8_t hours = 0;
        uint8_t day_low = 0;
        uint8_t day_high = 0;
    };

    //State
    bool ram_enabled = false;
    uint8_t current_rom_bank = 1;
    uint8_t selected_ram_or_rtc = 0;
    bool latch_armed = false;
    RtcRegisters rtc = {};
    RtcRegisters latched_rtc = {};

    [[nodiscard]] bool rtc_register_selected() const;
    [[nodiscard]] uint8_t read_rtc() const;
    void write_rtc(uint8_t value);
};

class MBC5 : public Cartridge {
public:
    MBC5(std::vector<uint8_t> rom_data, std::unique_ptr<CartridgeInfo> in_cartridge_info);

    [[nodiscard]] uint8_t read(const uint16_t& address) const override;
    void write(const uint16_t& address, uint8_t value) override;

    [[nodiscard]] uint8_t read_sram(uint16_t addr) const override;
    void write_sram(uint16_t addr, uint8_t value) override;

private:
    std::vector<uint8_t> rom;
    std::vector<uint8_t> ram;

    bool ram_enabled = false;
    uint16_t current_rom_bank = 1;
    uint8_t current_ram_bank = 0;
    bool rumble_enabled = false;
};
