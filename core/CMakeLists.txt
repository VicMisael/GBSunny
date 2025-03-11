add_library(CoreModule STATIC gb.cpp
        cpu/cpu.cpp
        cpu/cpu.h
        cpu/register_file.cpp
        cpu/register_file.h
        mmu/mmu.cpp
        mmu/mmu.h
        cpu/opcode_names.h
        cpu/cpu_opcodes.cpp
        utils/utils.h

)


target_include_directories(CoreModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})