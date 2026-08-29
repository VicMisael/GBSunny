cmake_minimum_required(VERSION 3.22)

project(GBSunnyPS2 LANGUAGES C CXX)

if(NOT PS2)
    message(FATAL_ERROR
        "GBSunnyPS2 must be configured with the PS2DEV toolchain: "
        "-DCMAKE_TOOLCHAIN_FILE=$ENV{PS2DEV}/share/ps2dev.cmake"
    )
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GBSUNNY_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

find_package(SDL2 CONFIG REQUIRED)

add_subdirectory(
    "${GBSUNNY_ROOT}/core"
    "${CMAKE_CURRENT_BINARY_DIR}/core"
)

add_executable(GBSunnyPS2
    component_performance_visitor.cpp
    component_performance_visitor.h
    main_ps2.cpp
    performance_overlay.cpp
    performance_overlay.h
    ps2_font.cpp
    ps2_font.h
    ps2_emulator.cpp
    ps2_emulator.h
    rom_selector.cpp
    rom_selector.h
)

set_target_properties(GBSunnyPS2 PROPERTIES
    OUTPUT_NAME "gbsunny"
    SUFFIX ".elf"
)

target_compile_features(GBSunnyPS2 PRIVATE cxx_std_23)

target_include_directories(GBSunnyPS2 PRIVATE
    "${GBSUNNY_ROOT}"
)

target_link_libraries(GBSunnyPS2 PRIVATE
    CoreModule
    SDL2::SDL2main
    SDL2::SDL2

    gskit
    dmakit
    gskit_toolkit
    pad
    audsrv
    fileXio
    patches
)
