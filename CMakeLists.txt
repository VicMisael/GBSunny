cmake_minimum_required(VERSION 3.22)
project(GBAsunny)

set(CMAKE_CXX_STANDARD 23)
find_package(raylib CONFIG REQUIRED)
add_subdirectory(core)



add_executable(GBAsunny main.cpp)
target_link_libraries(GBAsunny PRIVATE CoreModule
        raylib
)