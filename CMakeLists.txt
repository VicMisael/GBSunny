    cmake_minimum_required(VERSION 3.22)
project(GBAsunny)

set(CMAKE_CXX_STANDARD 23)
find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_path(PORTABLE_FILE_DIALOGS_INCLUDE_DIRS "portable-file-dialogs.h")
add_subdirectory(core)



add_executable(GBAsunny main.cpp)
target_link_libraries(GBAsunny PRIVATE CoreModule
        raylib
        glfw
)
target_include_directories(GBAsunny PRIVATE ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS})
