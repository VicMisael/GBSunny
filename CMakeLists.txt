cmake_minimum_required(VERSION 3.22)
project(GBAsunny)

set(CMAKE_CXX_STANDARD 23)
find_package(raylib CONFIG REQUIRED)
add_subdirectory(core)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

add_executable(GBAsunny main.cpp)
target_link_libraries(GBAsunny CoreModule  raylib)