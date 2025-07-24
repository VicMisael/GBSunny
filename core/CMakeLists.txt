add_library(CoreModule STATIC gb.cpp
        cpu/cpu.cpp
        cpu/cpu.h
        cpu/register_file.cpp
        cpu/register_file.h
        mmu/MMU.cpp
        mmu/MMU.h
        cpu/opcode_names.h
        cpu/cpu_opcodes.cpp
        utils/utils.h
        cpu/opcode_cycles.h
        ppu/scanline_ppu/ppu_scanline.cpp
        ppu/scanline_ppu/ppu_scanline.h
        ppu/fifo_ppu/ppu_fifo.h
        ppu/fifo_ppu/ppu_fifo.cpp
        cartridge/cartridge_info.h
        cartridge/cartridge_info.cc
        cartridge/cartridge.cc
        cartridge/cartridge.h
        cartridge/modes/mbc1.cpp
        cartridge/modes/mbc2.cpp
        cartridge/modes/mbc3.cpp
        cartridge/boot_rom.h
        timer/gb_timer.cpp
        timer/gb_timer.h
        spu/spu.cpp
        spu/spu.h
        cpu/register_types.h
        shared/types.h
        shared/interrupt.h
        shared/interrupt.cpp
        ppu/ppu_base.h)


target_include_directories(CoreModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})