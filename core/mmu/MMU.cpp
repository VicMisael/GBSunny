#include "MMU.h"

#include <iomanip>
#include <sstream>

#include "cartridge/boot_rom.h"
#include "spu/spu.h"
#include "utils/compiler.h"
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

constexpr std::size_t page_index(uint16_t address) {
	return address / mmu::page_size;
}

constexpr std::size_t mapped_page_count(uint16_t start, uint16_t end) {
	return (static_cast<std::size_t>(end) - start + 1) / mmu::page_size;
}

constexpr uint16_t ECHO_WRAMX_START = ECHO_START + (WRAM0_END - WRAM0_START + 1);

void mmu::MMU::reset() {
	bootRomControl = 0;
	init_read_mem_map();
	_serial->reset();
	_joypad->reset();

}
#pragma region MemMap

void mmu::MMU::map_read_only_page(std::size_t page, const uint8_t* block) {
	read_mem_regions[page] = block;
}

void mmu::MMU::on_boot_rom_control_update()
{
	if (bootRomControl != 0) {
			on_rom0_bank_update();
	}
}

void mmu::MMU::on_rom0_bank_update() {
	if (bootRomControl == 0) {
		return;
	}

	constexpr std::size_t rom0_start_page = page_index(ROM0_START);
	const auto* base_page = _cartridge->rom0_data();
	for (std::size_t page = 0; page < mapped_page_count(ROM0_START, ROM0_END); ++page) {
		map_read_only_page(
			rom0_start_page + page,
			base_page ? base_page + (page * page_size) : nullptr
		);
	}
}

void mmu::MMU::on_romx_bank_update() {
	constexpr std::size_t romx_start_page = page_index(ROMX_START);
	const auto* base_page = _cartridge->romx_data();
	for (std::size_t page = 0; page < mapped_page_count(ROMX_START, ROMX_END); ++page) {
		map_read_only_page(
			romx_start_page + page,
			base_page ? base_page + (page * page_size) : nullptr
		);
	}
}

void mmu::MMU::on_rom_bank_swap()
{
}

void mmu::MMU::on_ppu_vram_access_set(bool enable)
{
	const auto vram = _ppu->get_vram_ptr();
	if (enable)
	{
		for (size_t page = 0; page < mapped_page_count(VRAM_START, VRAM_END); ++page) {
			map_read_only_page(page_index(VRAM_START) + page, vram + (page * page_size));
		}
		return;
	}

	for (size_t page = 0; page < mapped_page_count(VRAM_START, VRAM_END); ++page) {
		map_read_only_page(page_index(VRAM_START) + page, blocked_memory_page.data());
	}
}

void mmu::MMU::on_ppu_dma(bool active)
{
	dma_active = active;
}

void mmu::MMU::init_read_mem_map()
{
	read_mem_regions.fill({});
	//ROM 0

	map_read_only_page(page_index(ROM0_START), cartridge::bootDMG.data());
	on_romx_bank_update();

	for (size_t page = 0; page < mapped_page_count(WRAM0_START, WRAM0_END); ++page) {
		map_read_only_page(page_index(WRAM0_START) + page, internal_RAM.data() + (page * page_size));
		map_read_only_page(page_index(WRAMX_START) + page, internal_RAM2.data() + (page * page_size));
		map_read_only_page(page_index(ECHO_START) + page, internal_RAM.data() + (page * page_size));
	}

	for (size_t page = 0; page < mapped_page_count(ECHO_WRAMX_START, ECHO_END); ++page) {
		map_read_only_page(page_index(ECHO_WRAMX_START) + page, internal_RAM2.data() + (page * page_size));
	}



	// Map according to the PPU's current access state.
	on_ppu_vram_access_set(true);

	//VRAM
	 
}

#pragma endregion MemMap

#pragma region MemoryReadAndWrite

uint8_t mmu::MMU::read(uint16_t addr) const
{
	if (slowReadPath) [[unlikely]]
		return read_slow(addr);

	if (dma_active) [[unlikely]] {
		if (addr < HRAM_START || addr > HRAM_END)
			return 0xFF;
	}

	const auto& page = read_mem_regions[addr >> 8];

	if (!page) [[unlikely]]
		return read_slow(addr);

	return page[addr & 0xFF];
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


NO_INLINE uint8_t mmu::MMU::read_slow(uint16_t addr) const {
	const auto region = decode_region(addr);


	if (dma_active && region != MemRegion::HRAM) {
		return 0xFF;
	}

	switch (region) {
	case MemRegion::ROM0:
	case MemRegion::ROMX:
		if (!bootRomControl && addr < 0x100) {
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
		return read(addr-0x2000);
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
		break;
	}

	std::ostringstream message;
	message << "Illegal Access: 0x" << std::hex << std::uppercase << addr;
	_logger->warning(message.str());

	return 0xFF;
}


void mmu::MMU::write(uint16_t addr, const uint8_t& data) {

	const auto region = decode_region(addr);


	if (dma_active && region != MemRegion::HRAM) {
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

}
#pragma endregion MemoryReadAndWrite

// --- I/O Register Handling ---


uint8_t mmu::MMU::io_read(uint16_t addr) const {

	switch (addr) {
	case 0xFF00: /* Joypad */
		return _joypad->read();
	case 0xFF0F:
		return read_interrupt_flag();
	case 0xFF4F: /* VRAM BANK SELECT */
		return 0xFE;
	case 0xFF50: /* Boot ROM disable register */
		return bootRomControl;
	case 0xFF70: /* WRAM BANK SELECT */
		return 0xF8;
	default:
		break;
	}

	if (utils::in_range(0xFF01, 0xFF02, addr)) {
		return _serial->read(addr);
	}
	if (utils::in_range(0xFF04, 0xFF07, addr)) {
		return _timer->read(addr);
	}
	if (utils::in_range(0xFF10, 0xFF26, addr)) {
		return _spu->read(addr);
	}
	if (utils::in_range(0xFF30, 0xFF3F, addr)) {
		return _spu->read_wave(addr);
	}
	if (utils::in_range(0xFF40, 0xFF4B, addr)) {
		return _ppu->read_control(addr);
	}
	if (utils::in_range(0xFF51, 0xFF55, addr)) { /* VRAM DMA */
		return 0xFF;
	}
	if (utils::in_range(0xFF68, 0xFF6B, addr)) { /* BG/OBJ Palette */
		return 0xFF;
	}

	return 0xFF; // Open bus behavior for unmapped I/O registers
}

void mmu::MMU::io_write(uint16_t addr, uint8_t data) {
	switch (addr) {
	case 0xFF00: /* Joypad */
		_joypad->write(data);
		return;
	case 0xFF4F: /* VRAM BANK SELECT */
	case 0xFF70: /* WRAM BANK SELECT */
		return;
	case 0xFF0F:
		set_interrupt_flag(data);
		return;
	case 0xFF50: /* Boot ROM disable register */
		bootRomControl = data;
		on_boot_rom_control_update();
		return;
	default:
		break;
	}

	if (utils::in_range(0xFF01, 0xFF02, addr)) {
		_serial->write(addr, data);
		return;
	}
	if (utils::in_range(0xFF04, 0xFF07, addr)) {
		_timer->write(addr, data);
		return;
	}
	if (utils::in_range(0xFF10, 0xFF26, addr)) {
		_spu->write(addr, data);
		return;
	}
	if (utils::in_range(0xFF30, 0xFF3F, addr)) {
		_spu->write_wave(addr, data);
		return;
	}
	if (utils::in_range(0xFF40, 0xFF4B, addr)) {
		if (addr == 0xFF46) {
			oam_transfer(data);
		}
		_ppu->write_control(addr, data); //Write Control signals to PPU
		return;
	}
	if (utils::in_range(0xFF51, 0xFF55, addr)) { /* VRAM DMA */
		return;
	}
	if (utils::in_range(0xFF68, 0xFF6B, addr)) { /* BG/OBJ Palette */
		return;
	}
}


// --- Interrupt Flag Handling ---

uint8_t mmu::MMU::read_interrupt_enable() const {
	return interrupt->enable.flag;
}

uint8_t mmu::MMU::read_interrupt_flag() const {
	return interrupt->requested.flag | 0xE0;
}

void mmu::MMU::set_interrupt_enable(uint8_t enable) {
	interrupt->enable.flag = enable & 0x1F; // Only lower 5 bits are used
}

void mmu::MMU::set_interrupt_flag(uint8_t input) {
	interrupt->requested.flag = input & 0x1F; // Only lower 5 bits are used
}

 void mmu::MMU::oam_transfer(const uint8_t params) const
{
	const auto ppu = this->_ppu;
	for (uint8_t i = 0; i < 0xA0; i++) {
		const auto address = utils::uint16_little_endian(i, params);
		ppu->write_oam(0xfe00 + i, this->read(address));
	}

}
