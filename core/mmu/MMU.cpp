//
// Created by Misael on 08/03/2025.
//

#include "MMU.h"

constexpr uint16_t ROM0_START   = 0x0000;
constexpr uint16_t ROM0_END     = 0x3FFF;

constexpr uint16_t ROMX_START   = 0x4000;
constexpr uint16_t ROMX_END     = 0x7FFF;

constexpr uint16_t VRAM_START   = 0x8000;
constexpr uint16_t VRAM_END     = 0x9FFF;

constexpr uint16_t SRAM_START   = 0xA000;
constexpr uint16_t SRAM_END     = 0xBFFF;

constexpr uint16_t WRAM0_START  = 0xC000;
constexpr uint16_t WRAM0_END    = 0xCFFF;

constexpr uint16_t WRAMX_START  = 0xD000;
constexpr uint16_t WRAMX_END    = 0xDFFF;

constexpr uint16_t ECHO_START   = 0xE000;
constexpr uint16_t ECHO_END     = 0xFDFF;

constexpr uint16_t OAM_START    = 0xFE00;
constexpr uint16_t OAM_END      = 0xFE9F;

constexpr uint16_t UNUSED_START = 0xFEA0;
constexpr uint16_t UNUSED_END   = 0xFEFF;

constexpr uint16_t IO_REG_START = 0xFF00;
constexpr uint16_t IO_REG_END   = 0xFF7F;

constexpr uint16_t HRAM_START   = 0xFF80;
constexpr uint16_t HRAM_END     = 0xFFFE;

constexpr uint16_t IE_REGISTER  = 0xFFFF;
//TODO
uint8_t mmu::MMU::read(uint16_t addr) {
	return 0;
}

uint8_t& mmu::MMU::read_as_ref(uint16_t addr) {
	return internal_RAM[0];
}

//TODO
void mmu::MMU::write(uint16_t addr,const uint8_t& data) {

}