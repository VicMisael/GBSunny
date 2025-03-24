#include "MMU.h"
//
// Created by Misael on 08/03/2025.
//


#include "cartridge/boot_rom.h"
#include "spu/spu.h"
#include "utils/utils.h"

// https://gbdev.io/pandocs/Memory_Map.html
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






void mmu::MMU::io_write(uint16_t addr, uint8_t data)
{
	const uint16_t index = addr & 0xFF;
	if (index == 0x0)
	{
		//JoyPad
	}
	if (utils::in_range(0x01, 0x02, addr))
	{
		//Serial transfer
	}
	if (utils::in_range(0x04, 0x07, addr))
	{
		_timer->write(addr,data);
		//Timer
	}
	if (addr == 0x0f)
	{
		set_interrupt_flag(data);
	}
	if (utils::in_range(0x10, 0x26, addr))
	{
		 _spu->write(addr,data);
	}
	if (utils::in_range(0x30, 0x3f, addr))
	{
		 _spu->write_wave(addr,data);
	}
	if (utils::in_range(0x40, 0x4b, addr))
	{
		_ppu->write_ppucontrol(addr,data);
	}

	if (addr == 0x50)
	{
		bootRomControl = data;
	}
	//TODO all of the ones below are CGB specifica
	if (addr == 0x4f)
	{
		//VRAM BANK SELECT
	}
	if (utils::in_range(0x51, 0x55, addr))
	{
		// VRAM DMQA
	}
	if (utils::in_range(0x68, 0x6b, addr))
	{
		// BG/OBJ
	}
	if (addr == 0x70)
	{
		// WRAM BANK SELECT
	}
}

bool mmu::MMU::boot_rom_enabled() const
{
	return bootRomControl==0;
}


void mmu::MMU::reset()
{
}

uint8_t mmu::MMU::read(uint16_t addr) const{
	if (utils::in_range(ROM0_START, ROMX_END, addr))
	{
		if (boot_rom_enabled())
		{
			return cartridge::bootDMG[addr];
		}
		return cartridge->read(addr);
	}

	if (utils::in_range(VRAM_START, VRAM_END, addr))
	{
		return _ppu->read(addr);
	}

	if (utils::in_range(SRAM_START, SRAM_END, addr))
	{
		return cartridge->read_sram(addr);
	}
	if (utils::in_range(WRAM0_START, WRAM0_END, addr))
	{
		return internal_RAM[addr&0xfff];
	}
	if (utils::in_range(WRAMX_START, WRAMX_END, addr))
	{
		return internal_RAM2[addr&0xfff];
	}
	if (utils::in_range(ECHO_START, ECHO_END, addr))
	{
		return internal_RAM[addr-0x2000];
	}
	if (utils::in_range(OAM_START, OAM_END, addr))
	{
		return _ppu->read_oam(addr);
	}
	if (utils::in_range(UNUSED_START, UNUSED_END, addr))
	{
		return 0xff;
	}
	if (utils::in_range(IO_REG_START, IO_REG_END, addr))
	{
		return io_read(addr);
	}
	if (utils::in_range(HRAM_START,HRAM_END, addr))
	{
		return HRAM[addr-HRAM_START];
	}
	if (addr==IE_REGISTER)
	{
		return this->read_interrupt_enable();
	}
	throw std::runtime_error("Invalid ROM address");
}

uint8_t mmu::MMU::read_interrupt_enable() const
{
	return this->interrupt->enable.flag;
}

uint8_t mmu::MMU::read_interrupt_flag() const
{
	return interrupt->flag.flag;
}

void mmu::MMU::set_interrupt_flag(uint8_t input)
{
	interrupt->flag.flag = input&0x1f;
}

void mmu::MMU::set_interrupt_enable(uint8_t enable)
{
	interrupt->enable.flag = enable&0x1f;//Ignore the 3 bits
}


uint8_t mmu::MMU::io_read(uint16_t addr) const
{
	const uint16_t index = addr & 0xFF;
	if (index==0x0)
	{
		//JoyPad
	}
	if (utils::in_range(0x01,0x02,addr))
	{
		//Serial transfer
	}
	if (utils::in_range(0x04,0x07,addr))
	{
		return _timer->read(addr);
		//Timer
	}
	if (addr==0x0f)
	{
		return read_interrupt_flag();
	}
	if (utils::in_range(0x10,0x26,addr))
	{
		return _spu->read(addr);
	}
	if (utils::in_range(0x30,0x3f,addr))
	{
		return _spu->read_wave(addr);
	}
	if (utils::in_range(0x40,0x4b,addr))
	{
		return _ppu->read_ppucontrol(addr);
	}

	if (addr==0x50)
	{
		return bootRomControl;
	}
	//TODO all of the ones below are CGB specific
	if (addr==0x4f)
	{
		//VRAM BANK SELECT
	}
	if (utils::in_range(0x51,0x55,addr))
	{
		// VRAM DMQA
	}
	if (utils::in_range(0x68,0x6b,addr))
	{
		// BG/OBJ
	}
	if (addr==0x70)
	{
		// WRAM BANK SELECT
	}
	return 0xff;


}

//TODO
void mmu::MMU::write(uint16_t addr,const uint8_t& data) {
	if (utils::in_range(ROM0_START, ROMX_END, addr))
	{
		 cartridge->write(addr,data);
	}

	if (utils::in_range(VRAM_START, VRAM_END, addr))
	{
		return _ppu->write(addr,data);
	}

	if (utils::in_range(SRAM_START, SRAM_END, addr))
	{
		 cartridge->write_sram(addr);
	}
	if (utils::in_range(WRAM0_START, WRAM0_END, addr))
	{
		internal_RAM[addr&0xfff] = data;
	}
	if (utils::in_range(WRAMX_START, WRAMX_END, addr))
	{
		 internal_RAM2[addr&0xfff] =data;
	}
	if (utils::in_range(ECHO_START, ECHO_END, addr))
	{
		 internal_RAM[addr - ECHO_START] =data;
	}
	if (utils::in_range(OAM_START, OAM_END, addr))
	{
		 _ppu->write_oam(addr,data);
	}
	if (utils::in_range(UNUSED_START, UNUSED_END, addr))
	{
		return;
	}
	if (utils::in_range(IO_REG_START, IO_REG_END, addr))
	{
		 io_write(addr,data);
	}
	if (utils::in_range(HRAM_START,HRAM_END, addr))
	{
		 HRAM[addr-HRAM_START] = data;
	}
	if (addr==IE_REGISTER)
	{
		this->set_interrupt_enable(data);
	}
	throw std::runtime_error("Invalid ROM address");
}