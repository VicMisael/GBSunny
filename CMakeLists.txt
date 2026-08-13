    cmake_minimum_required(VERSION 3.22)
project(GBAsunny)

set(CMAKE_CXX_STANDARD 23)
find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
add_subdirectory(core)



add_executable(GBAsunny
        main.cpp
        frontend/frontend.cpp
        frontend/frontend.h
        frontend/audio_ring_buffer.h
        utils/file_dialog.cpp
)

target_include_directories(GBAsunny PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(GBAsunny PRIVATE CoreModule
        raylib
        glfw
)
