//
// Created by Misael on 08/03/2025.
//

#ifndef MMU_H
#define MMU_H
#include <cstdint>
#include <memory>
#include <array>
#include <algorithm>

#include "cartridge/cartridge.h"
#include "blocked_memory_page.h"
#include "memory_page.h"
#include "joypad/joypad.h"
#include "spu/spu.h"
#include "timer/gb_timer2.h"
#include "shared/interrupt.h"
#include "serial/gb_serial.h"
#include "logging/core_logger.h"
#include <ppu/ppu_base.h>



namespace mmu {
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
        std::shared_ptr<spu> _spu;
        std::shared_ptr<serial::GBSerial> _serial;
        std::shared_ptr<Joypad> _joypad;
        std::shared_ptr<logging::CoreLogger> _logger;
        std::shared_ptr<shared::interrupt> interrupt; //Shared space for interrupts

        std::array<const uint8_t*, page_count> read_mem_regions{};

        [[nodiscard]] uint8_t read_interrupt_enable() const;
        [[nodiscard]] uint8_t read_interrupt_flag() const;
        void set_interrupt_flag(uint8_t);
        void set_interrupt_enable(uint8_t);

        void oam_transfer(uint8_t params) const;


        [[nodiscard]] uint8_t io_read(uint16_t addr) const;
        [[nodiscard]] uint8_t read_slow(uint16_t addr) const;
        void io_write(uint16_t addr, uint8_t data);

    public:
        MMU(const std::shared_ptr<Cartridge>& cart,
            std::shared_ptr<PPU_Base> ppu_ptr,
            const std::shared_ptr<base_timer>& timer_ptr,
            const std::shared_ptr<shared::interrupt>& interrupt_ptr,
            const std::shared_ptr<spu>& spu_ptr,
            std::shared_ptr<serial::GBSerial> serial_ptr,
            std::shared_ptr<Joypad> joypad_ptr,
            std::shared_ptr<logging::CoreLogger> logger = nullptr,
            bool use_slow_read_path = true

        ) : slowReadPath(use_slow_read_path),
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

        [[nodiscard]] uint8_t read(uint16_t addr) const ;

        void write(uint16_t addr, const uint8_t &data);

#pragma region Memory Mapping
        void map_read_only_page(std::size_t page, const uint8_t* block);
        void on_boot_rom_control_update();

        void on_rom0_bank_update();

        void on_romx_bank_update();

        void on_rom_bank_swap();
        void on_ppu_vram_access_set(bool enable);
        void on_ppu_dma(bool active);
#pragma endregion Memory Mapping

    };
};


#endif //MMU_H
