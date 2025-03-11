//
// Created by Misael on 08/03/2025.
//

#include "mmu.h"

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
uint8_t& mmu::mmu::read(uint16_t addr) {
    uint8_t result = 0;
    //switch (addr) {
    //    case ROM0_START ... ROMX_END:
    //        return 0xFF;
    //    case VRAM_START ... VRAM_END:
    //        return video_RAM[addr - VRAM_START];
    //    case SRAM_START ... SRAM_END:
    //        return 0;
    //    case WRAM0_START ... WRAM0_END:
    //    case WRAMX_START ... WRAMX_END:
    //        return internal_RAM[addr - WRAM0_START];
    //    case ECHO_START ... ECHO_END:
    //        return internal_RAM[addr - ECHO_START];
    //    case OAM_START ... OAM_END:
    //         // OAM access
    //    case UNUSED_START ... UNUSED_END:
    //        // Unused memory
    //    case IO_REG_START ... IO_REG_END:
    //        // return io_registers[addr - IO_REG_START];
    //    case HRAM_START ... HRAM_END:
    //        // return hram[addr - HRAM_START];
    //    case IE_REGISTER:
    //        // return *interrupt_enable;
    //    default:
    //        return 0xff;
    //}

    if (addr >= ROM0_START && addr <= ROMX_END) {
        result= 0xFF;
    }
    else if (addr >= VRAM_START && addr <= VRAM_END) {
        return video_RAM[addr - VRAM_START];
    }
    else if (addr >= SRAM_START && addr <= SRAM_END) {
        result = 0;
    }
    else if ((addr >= WRAM0_START && addr <= WRAM0_END) || (addr >= WRAMX_START && addr <= WRAMX_END)) {
        return internal_RAM[addr - WRAM0_START];
    }
    else if (addr >= ECHO_START && addr <= ECHO_END) {
        return internal_RAM[addr - ECHO_START];
    }
    else if (addr >= OAM_START && addr <= OAM_END) {
        // OAM access placeholder
    }
    else if (addr >= UNUSED_START && addr <= UNUSED_END) {
        // Unused memory placeholder
    }
    else if (addr >= IO_REG_START && addr <= IO_REG_END) {
        // return io_registers[addr - IO_REG_START];
    }
    else if (addr >= HRAM_START && addr <= HRAM_END) {
        //return hram[addr - HRAM_START];
    }
    else if (addr == IE_REGISTER) {
        //return interrupt_enable;
    }

    return temp;
}
//TODO
void mmu::mmu::write(uint16_t addr,const uint8_t& data) {
    //switch (addr) {
    //    case ROM0_START ... ROMX_END:
    //        // return 0xFF;
    //    case VRAM_START ... VRAM_END:
    //         video_RAM[addr - VRAM_START]=data;
    //    case SRAM_START ... SRAM_END:
    //        // return 0;
    //    case WRAM0_START ... WRAM0_END:
    //    case WRAMX_START ... WRAMX_END:
    //         internal_RAM[addr - WRAM0_START]=data;
    //    case ECHO_START ... ECHO_END:
    //         internal_RAM[addr - ECHO_START]=data;
    //    case OAM_START ... OAM_END:
    //         // OAM access
    //    case UNUSED_START ... UNUSED_END:
    //        // Unused memory
    //    case IO_REG_START ... IO_REG_END:
    //        // return io_registers[addr - IO_REG_START];
    //    case HRAM_START ... HRAM_END:
    //        // return hram[addr - HRAM_START];
    //    case IE_REGISTER:
    //        // return *interrupt_enable;
    //    default:
    //        return;

    //}

    if (addr >= ROM0_START && addr <= ROMX_END) {
        return; // ROM is read-only
    }
    else if (addr >= VRAM_START && addr <= VRAM_END) {
        video_RAM[addr - VRAM_START] = data;
    }
    else if (addr >= SRAM_START && addr <= SRAM_END) {
        return;
    }
    else if ((addr >= WRAM0_START && addr <= WRAM0_END) || (addr >= WRAMX_START && addr <= WRAMX_END)) {
        internal_RAM[addr - WRAM0_START] = data;
    }
    else if (addr >= ECHO_START && addr <= ECHO_END) {
        internal_RAM[addr - ECHO_START] = data;
    }
    else if (addr >= OAM_START && addr <= OAM_END) {
        // OAM access placeholder
    }
    else if (addr >= UNUSED_START && addr <= UNUSED_END) {
        // Unused memory placeholder
    }
    else if (addr >= IO_REG_START && addr <= IO_REG_END) {
        // io_registers[addr - IO_REG_START] = data;
    }
    else if (addr >= HRAM_START && addr <= HRAM_END) {
        //hram[addr - HRAM_START] = data;
    }
    else if (addr == IE_REGISTER) {
        //interrupt_enable = data;
    }
}