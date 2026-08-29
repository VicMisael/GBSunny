add_library(CoreModule STATIC gb.cpp
        cpu/cpu.cpp
        cpu/cpu.h
        cpu/register_file.cpp
        cpu/register_file.h
        mmu/MMU.cpp
        mmu/MMU.h
        joypad/joypad.cpp
        joypad/joypad.h
        cpu/opcode_names.h
        cpu/cpu_opcodes.cpp
        utils/utils.h
        cpu/opcode_cycles.h
        ppu/scanline_ppu/ppu_scanline.cpp
        ppu/scanline_ppu/ppu_scanline.h
        ppu/tick_fifo_ppu/ppu_fifo_types.h
        ppu/tick_fifo_ppu/ppu_tick_fifo.h
        ppu/tick_fifo_ppu/ppu_tick_fifo.cpp
        cartridge/cartridge_info.h
        cartridge/cartridge_info.cc
        cartridge/cartridge.cc
        cartridge/cartridge.h
        cartridge/modes/mbc1.cpp
        cartridge/modes/mbc2.cpp
        cartridge/modes/mbc3.cpp
        cartridge/boot_rom.h
        timer/gb_timer2.cpp
        timer/gb_timer2.h
        timer/gb_timer.cpp
        timer/gb_timer.h
        spu/spu.cpp
        spu/SpuChannelRegisters.h
        spu/SpuRegisterFile.h
        "spu/spu.h"
        serial/gb_serial.cpp
        serial/gb_serial.h
        cpu/register_types.h
        shared/types.h
        shared/interrupt.h
        shared/interrupt.cpp
        ppu/ppu_base.h
 timer/base_timer.h 
 cartridge/modes/mbc5.cpp events/event_aggregator.h events/events.h
        logging/core_logger.h
        profiling/gb_component_observer.h
        utils/emu_flags.h
        utils/compiler.h)


target_include_directories(CoreModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
