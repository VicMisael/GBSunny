#include "MMU.h"

#include <iostream>

#include "cartridge/boot_rom.h"
#include "spu/spu.h"
#include "utils/utils.h"

// Memory Map Constants
// https://gbdev.io/pandocs/Memory_Map.html
constexpr uint16_t ROM0_START = 0x0000;
constexpr uint16_t ROM0_END = 0x3FFF;
constexpr uint16_t ROMX_START = 0x4000;
constexpr uint16_t ROMX_END = 0x7FFF;
constexpr uint16_t VRAM_START = 0x8000;
constexpr uint16_t VRAM_END = 0x9FFF;
constexpr uint16_t SRAM_START = 0xA000;
constexpr uint16_t SRAM_END = 0xBFFF;
constexpr uint16_t WRAM0_START = 0xC000;
constexpr uint16_t WRAM0_END = 0xCFFF;
constexpr uint16_t WRAMX_START = 0xD000;
constexpr uint16_t WRAMX_END = 0xDFFF;
constexpr uint16_t ECHO_START = 0xE000;
constexpr uint16_t ECHO_END = 0xFDFF;
constexpr uint16_t OAM_START = 0xFE00;
constexpr uint16_t OAM_END = 0xFE9F;
constexpr uint16_t UNUSED_START = 0xFEA0;
constexpr uint16_t UNUSED_END = 0xFEFF;
constexpr uint16_t IO_REG_START = 0xFF00;
constexpr uint16_t IO_REG_END = 0xFF7F;
constexpr uint16_t HRAM_START = 0xFF80;
constexpr uint16_t HRAM_END = 0xFFFE;
constexpr uint16_t IE_REGISTER = 0xFFFF;

void mmu::MMU::reset() {
	bootRomControl = 0;
}

bool mmu::MMU::boot_rom_enabled() const {
	return bootRomControl == 0;
}

enum class MemRegion {
	ROM0, ROMX, VRAM, SRAM,
	WRAM0, WRAMX, ECHO,
	OAM, UNUSED, IO, HRAM,
	IE, INVALID
};


constexpr static MemRegion decode_region(uint16_t addr) {
	static constexpr std::array region_lut = {
		MemRegion::ROM0,  MemRegion::ROM0,  MemRegion::ROM0,  MemRegion::ROM0,
		MemRegion::ROMX,  MemRegion::ROMX,  MemRegion::ROMX,  MemRegion::ROMX,
		MemRegion::VRAM,  MemRegion::VRAM,
		MemRegion::SRAM,  MemRegion::SRAM,
		MemRegion::WRAM0,
		MemRegion::WRAMX,
		MemRegion::ECHO,  MemRegion::ECHO
	};

	// For the majority of addresses, a single array lookup is sufficient.
	if (addr < 0xFE00) {
		return region_lut[addr >> 12];
	}

	// Handle the small, irregular regions at the top of memory.
	if (addr < 0xFEA0) return MemRegion::OAM;    // 0xFE00 - 0xFE9F
	if (addr < 0xFF00) return MemRegion::UNUSED; // 0xFEA0 - 0xFEFF
	if (addr < 0xFF80) return MemRegion::IO;     // 0xFF00 - 0xFF7F
	if (addr < 0xFFFF) return MemRegion::HRAM;   // 0xFF80 - 0xFFFE

	return MemRegion::IE; // 0xFFFF
}




uint8_t mmu::MMU::read(uint16_t addr) const {

	const auto region = decode_region(addr);


	if (_ppu->is_dma_active() && region != MemRegion::HRAM) {
		return 0xFF;
	}

	switch (region) {
	case MemRegion::ROM0:
	case MemRegion::ROMX:
		if (boot_rom_enabled() && addr < 0x100) {
			return cartridge::bootDMG[addr];
		}
		return _cartridge->read(addr);
	case MemRegion::VRAM:
		return _ppu->is_vram_accessible() ? _ppu->read_vram(addr) : 0xFF;
	case MemRegion::SRAM:
		return _cartridge->read_sram(addr);
	case MemRegion::WRAM0:
		return internal_RAM[addr & 0x0FFF];
	case MemRegion::WRAMX:
		return internal_RAM2[addr & 0x0FFF]; // CGB
	case MemRegion::ECHO:
		return internal_RAM[(addr - 0x2000) & 0x0FFF];
	case MemRegion::OAM:
		return _ppu->is_oam_accessible() ? _ppu->read_oam(addr) : 0xFF;
	case MemRegion::UNUSED:
		return 0xFF;
	case MemRegion::IO:
		return io_read(addr);
	case MemRegion::HRAM:
		return HRAM[addr - HRAM_START];
	case MemRegion::IE:
		return read_interrupt_enable();
	default:
		std::cout << "Illegal Access: " << std::hex << addr << std::endl;
		return 0xFF;
	}



	
}




void mmu::MMU::write(uint16_t addr, const uint8_t& data) {

	const auto region = decode_region(addr);


	if (_ppu->is_dma_active() && region != MemRegion::HRAM) {
		return;
	}

	switch (region) {
	case MemRegion::ROM0:
	case MemRegion::ROMX: _cartridge->write(addr, data);  break;
	case MemRegion::VRAM: _ppu->write_vram(addr, data);  break;
	case MemRegion::SRAM:_cartridge->write_sram(addr, data);  break;
	case MemRegion::WRAM0:;
	case MemRegion::WRAMX:
	{
		if (addr < 0xD000) {
			internal_RAM[addr & 0x0FFF] = data;
		}
		else {
			internal_RAM2[addr & 0x0FFF] = data;
		}
	} break;
	case MemRegion::ECHO: internal_RAM[(addr - 0x2000) & 0x0FFF] = data;  break;
	case MemRegion::OAM:
	{
		if (_ppu->is_oam_accessible()) {
			_ppu->write_oam(addr, data);
		}
	} break;
	case MemRegion::UNUSED: break;
	case MemRegion::IO:io_write(addr, data); break;
	case MemRegion::HRAM:	HRAM[addr - HRAM_START] = data; break;
	case MemRegion::IE: set_interrupt_enable(data);  break;
	case MemRegion::INVALID: break;
	}
	return;

	// std::cout<<"MMU::MMU::write: "<<std::hex<<addr<<std::endl;
}


// --- I/O Register Handling ---


uint8_t mmu::MMU::io_read(uint16_t addr) const {
	if (addr == 0xFF00) { /* Joypad */ return 0xCF; } // Example default value
	if (addr == 0xFF01) return serial_data;
	if (addr == 0xFF02) return serial_control;
	if (utils::in_range(0xFF04, 0xFF07, addr)) { return _timer->read(addr); }
	if (addr == 0xFF0F) { return read_interrupt_flag(); }
	if (utils::in_range(0xFF10, 0xFF26, addr)) { return _spu->read(addr); }
	if (utils::in_range(0xFF30, 0xFF3F, addr)) { return _spu->read_wave(addr); }
	if (utils::in_range(0xFF40, 0xFF4B, addr)) { return _ppu->read_control(addr); }
	if (addr == 0xFF50) { return bootRomControl; }
	// CGB specific registers
	if (addr == 0xFF4F) { /* VRAM BANK SELECT */ return 0xFE; }
	if (utils::in_range(0xFF51, 0xFF55, addr)) { /* VRAM DMA */ return 0xFF; }
	if (utils::in_range(0xFF68, 0xFF6B, addr)) { /* BG/OBJ Palette */ return 0xFF; }
	if (addr == 0xFF70) { /* WRAM BANK SELECT */ return 0xF8; }

	return 0xFF; // Open bus behavior for unmapped I/O registers
}

void mmu::MMU::io_write(uint16_t addr, uint8_t data) {
	if (addr == 0xFF00) { /* Joypad */ }
	if (utils::in_range(0xFF01, 0xFF02, addr)) {
		if (addr == 0xFF01) {
			serial_data = data;
		}
		else if (addr == 0xFF02) {
			serial_control = data;

			// Inicia a transferência se o bit 7 for setado
			if (serial_control & 0x80) {
				//std::cout << "SERIAL OUT" << std::endl;
				// Imprime o byte enviado no terminal
				std::cout << serial_data;

				// Finaliza a "transferência" (bit 7 limpo)
				serial_control &= ~0x80;

				// Retorna 0xFF como byte recebido (dummy)
				serial_data = 0xFF;
			}
		}

	}
	if (utils::in_range(0xFF04, 0xFF07, addr)) { _timer->write(addr, data); }
	if (addr == 0xFF0F) { set_interrupt_flag(data); }
	if (utils::in_range(0xFF10, 0xFF26, addr)) { _spu->write(addr, data); }
	if (utils::in_range(0xFF30, 0xFF3F, addr)) { _spu->write_wave(addr, data); }
	if (utils::in_range(0xFF40, 0xFF4B, addr)) {
		if (addr == 0xFF46) {
			oam_transfer(data);
		}
		_ppu->write_control(addr, data); //Write Control signals to PPU
	}
	if (addr == 0xFF50) {
		bootRomControl = data;
	} // Boot ROM disable register
	// CGB specific registers
	if (addr == 0xFF4F) { /* VRAM BANK SELECT */ }
	if (utils::in_range(0xFF51, 0xFF55, addr)) { /* VRAM DMA */ }
	if (utils::in_range(0xFF68, 0xFF6B, addr)) { /* BG/OBJ Palette */ }
	if (addr == 0xFF70) { /* WRAM BANK SELECT */ }
}


// --- Interrupt Flag Handling ---

uint8_t mmu::MMU::read_interrupt_enable() const {
	return interrupt->enable.flag;
}

uint8_t mmu::MMU::read_interrupt_flag() const {
	return interrupt->flag.flag;
}

void mmu::MMU::set_interrupt_enable(uint8_t enable) {
	interrupt->enable.flag = enable & 0x1F; // Only lower 5 bits are used
}

inline void mmu::MMU::oam_transfer(const uint8_t params) const
{
	for (uint8_t i = 0; i <= 0xA0; i++) {
		const auto address = utils::uint16_little_endian(i, params);
		this->_ppu->write_oam(0xfe00 + i, this->read(address));
	}

}

void mmu::MMU::set_interrupt_flag(uint8_t input) {
	interrupt->flag.flag = input & 0x1F; // Only lower 5 bits are used
}
