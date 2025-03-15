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
        ppu/ppu.cpp
        ppu/ppu.h
        cartridge/cartridge_info.h
        cartridge/cartridge_info.cc
        cartridge/cartridge.cc
        cartridge/cartridge.h
        cartridge/boot_rom.h
        timer/timer.cpp
        timer/timer.h
        spu/spu.cpp
        spu/spu.h
        cpu/register_types.h


)


target_include_directories(CoreModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})