    cmake_minimum_required(VERSION 3.22)
project(GBAsunny)

set(CMAKE_CXX_STANDARD 23)
option(GBSUNNY_BUILD_BENCHMARKS "Build GBSunny microbenchmarks" ON)

include(CheckIPOSupported)
check_ipo_supported(RESULT GBSUNNY_IPO_SUPPORTED OUTPUT GBSUNNY_IPO_ERROR LANGUAGES CXX)

find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
add_subdirectory(core)

if(GBSUNNY_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

add_executable(GBAsunny
        main.cpp
        frontend/frontend.cpp
        frontend/frontend.h
        frontend/audio_ring_buffer.h
        frontend/toast.cpp
        frontend/toast.h
        frontend/ui.cpp
        frontend/ui.h
        utils/debug_utils.h
        utils/file_dialog.cpp
)


#target_compile_definitions(CoreModule PRIVATE READ_STATS)
# target_compile_definitions(CoreModule PRIVATE SLOW_MEM_READS)
target_include_directories(GBAsunny PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(GBAsunny PRIVATE CoreModule
        raylib
        glfw
)

if(GBSUNNY_IPO_SUPPORTED)
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET GBAsunny CoreModule PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${GBSUNNY_IPO_ERROR}>")
endif()
