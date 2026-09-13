//
// Created by Misael on 08/03/2025.
//

#ifndef MMU_H
#define MMU_H
#include <cstdint>
#include <memory>
#include <array>
#include <algorithm>
#include <cstring>

#include "cartridge/cartridge.h"
#include "blocked_memory_page.h"
#include "memory_page.h"
#include "joypad/joypad.h"
#include "spu/SPUBase.h"
#include "timer/gb_timer2.h"
#include "shared/interrupt.h"
#include "serial/gb_serial.h"
#include "logging/core_logger.h"
#include <ppu/ppu_base.h>



namespace mmu {
    enum class MemRegion : uint8_t {
        ROM0, ROMX, VRAM, SRAM,
        WRAM0, WRAMX, ECHO,
        OAM, UNUSED, IO, HRAM,
        IE, INVALID, COUNT
    };

    inline constexpr std::size_t MemRegionCount = static_cast<std::size_t>(MemRegion::COUNT);

    struct ReadStats {
        uint64_t total = 0;
        uint64_t slow = 0;
        uint64_t mapped = 0;
        uint64_t hram = 0;
        uint64_t dma_blocked = 0;
        std::array<uint64_t, MemRegionCount> slow_by_region{};
    };

    inline ReadStats read_stats;

    class MMU {

        std::array<uint8_t,4096> internal_RAM{};
        std::array<uint8_t,4096> internal_RAM2{};
        std::array<uint8_t,128> HRAM{};

        uint8_t bootRomControl = 0;
        bool slowReadPath = true;
		bool dma_active = false;
        std::shared_ptr<PPU_Base> _ppu;
        std::shared_ptr<base_timer> _timer;
        std::shared_ptr<Cartridge> _cartridge;
        std::shared_ptr<SPUBase> _spu;
        std::shared_ptr<serial::GBSerial> _serial;
        std::shared_ptr<Joypad> _joypad;
        std::shared_ptr<logging::CoreLogger> _logger;
        std::shared_ptr<shared::interrupt> interrupt; //Shared space for interrupts

        std::array<const uint8_t*, page_count> read_mem_regions{};
        std::array<uint8_t*, page_count> write_mem_regions{};

        [[nodiscard]] uint8_t read_interrupt_enable() const;
        [[nodiscard]] uint8_t read_interrupt_flag() const;
        void set_interrupt_flag(uint8_t);
        void set_interrupt_enable(uint8_t);

        void oam_transfer(uint8_t params) const;


        [[nodiscard]] uint8_t io_read(uint16_t addr) const;
        [[nodiscard]] uint8_t read_slow(uint16_t addr) const;
        void write_slow(uint16_t addr, const uint8_t& data);
        void io_write(uint16_t addr, uint8_t data);

    public:
        MMU(const std::shared_ptr<Cartridge>& cart,
            std::shared_ptr<PPU_Base> ppu_ptr,
            const std::shared_ptr<base_timer>& timer_ptr,
            const std::shared_ptr<shared::interrupt>& interrupt_ptr,
            const std::shared_ptr<SPUBase>& spu_ptr,
            std::shared_ptr<serial::GBSerial> serial_ptr,
            std::shared_ptr<Joypad> joypad_ptr,
            std::shared_ptr<logging::CoreLogger> logger = nullptr

        ) :
            _ppu(std::move(ppu_ptr)), _timer(timer_ptr), _cartridge(cart),
            _spu(spu_ptr),
            _serial(std::move(serial_ptr)),
            _joypad(std::move(joypad_ptr)),
            _logger(std::move(logger)),
            interrupt(interrupt_ptr) {
            if (_logger == nullptr) {
                _logger = std::make_shared<logging::NullCoreLogger>();
            }
			_ppu->set_vram_access_callback([this](bool enable) {
				on_ppu_vram_access_set(enable);
			});
			_ppu->set_dma_callback([this](bool active) {
				on_ppu_dma(active);
			});
			_cartridge->set_rom0_bank_update_callback([this] {
				on_rom0_bank_update();
			});
			_cartridge->set_romx_bank_update_callback([this] {
				on_romx_bank_update();
			});
            init_read_mem_map();
        }

		~MMU() {
			if (_ppu) {
				_ppu->set_vram_access_callback({});
				_ppu->set_dma_callback({});
			}
			if (_cartridge) {
				_cartridge->set_rom0_bank_update_callback({});
				_cartridge->set_romx_bank_update_callback({});
			}
		}

        void reset();

        void init_read_mem_map();

        [[nodiscard]] uint8_t read(uint16_t addr) const
        {
#ifdef READ_STATS
            read_stats.total++;
#endif
#ifdef SLOW_MEM_READS
            return read_slow(addr);
#endif

            constexpr uint16_t hram_start = 0xFF80;
            constexpr uint16_t hram_end = 0xFFFE;

            if (dma_active && (addr < hram_start || addr > hram_end)) [[unlikely]] {
#ifdef READ_STATS
                read_stats.dma_blocked++;
#endif
                return 0xFF;
            }

            const auto* mapped_page = read_mem_regions[addr >> 8];
            if (mapped_page != nullptr) [[likely]] {
#ifdef READ_STATS
                read_stats.mapped++;
#endif
                return mapped_page[addr & 0xFF];
            }

            if (addr >= hram_start && addr <= hram_end) {
#ifdef READ_STATS
                read_stats.hram++;
#endif
                return HRAM[addr - hram_start];
            }

            return read_slow(addr);
        }

		[[nodiscard]] uint16_t read16(uint16_t addr) const
        {
#ifndef SLOW_MEM_READS
            if (!dma_active && (addr & 0xFF) != 0xFF) [[likely]] {
                const auto* page = read_mem_regions[addr >> 8];

                if (page != nullptr) [[likely]] {
#ifdef READ_STATS
                    read_stats.total += 2;
                    read_stats.mapped += 2;
#endif
                    std::uint16_t value;
                    std::memcpy(&value, page + (addr & 0xFF), sizeof(value));
                    return value;
                }
            }
#endif

            const uint8_t low = read(addr);
            const uint8_t high = read(static_cast<uint16_t>(addr + 1));
            return static_cast<uint16_t>(low | (static_cast<uint16_t>(high) << 8));
        }

        void write(uint16_t addr, const uint8_t &data)
        {
            constexpr uint16_t hram_start = 0xFF80;
            constexpr uint16_t hram_end = 0xFFFE;

            if (dma_active && (addr < hram_start || addr > hram_end)) [[unlikely]] {
                return;
            }

            if (auto* mapped_page = write_mem_regions[addr >> 8]; mapped_page != nullptr) [[likely]] {
                mapped_page[addr & 0xFF] = data;
                return;
            }

            if (addr >= hram_start && addr <= hram_end) {
                HRAM[addr - hram_start] = data;
                return;
            }

            write_slow(addr, data);
        }

		[[nodiscard]] uint8_t* get_writable_memory_block(uint16_t addr)
		{
			constexpr uint16_t HRAM_START = 0xFF80;
			constexpr uint16_t HRAM_END = 0xFFFE;

			if (dma_active && (addr < HRAM_START || addr > HRAM_END)) [[unlikely]] {
				return nullptr;
			}

			if (addr >= HRAM_START && addr <= HRAM_END) {

				return &HRAM[addr - HRAM_START];
			}
			auto writeMem = write_mem_regions[addr >> 8];
			if (writeMem == nullptr)
			{
				return nullptr;
			}
			return &writeMem[addr&0xff];

		};

        [[nodiscard]] static ReadStats get_read_stats();

#pragma region Memory Mapping
        void map_read_only_page(std::size_t page, const uint8_t* block);
        void map_write_page(std::size_t page, uint8_t* block);
        void on_boot_rom_control_update();

        void on_rom0_bank_update();

        void on_romx_bank_update();
    	
        void on_ppu_vram_access_set(bool enable);
        void on_ppu_dma(bool active);
#pragma endregion Memory Mapping

    };
};


#endif //MMU_H
