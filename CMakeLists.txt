cmake_minimum_required(VERSION 3.22)
project(GBAsunny)

set(CMAKE_CXX_STANDARD 23)
find_package(SDL2 CONFIG REQUIRED)
add_subdirectory(core)



add_executable(GBAsunny main.cpp)
target_link_libraries(GBAsunny PRIVATE CoreModule     $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)