#include "rom_selector.h"

#include <SDL.h>
#include <ps2_filesystem_driver.h>

#include <cstdio>

int main(int, char**)
{
    init_ps2_filesystem_driver();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::printf("SDL_Init failed: %s\n", SDL_GetError());
        deinit_ps2_filesystem_driver();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "GBSunny",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        640,
        448,
        SDL_WINDOW_SHOWN);

    if (window == nullptr) {
        std::printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        deinit_ps2_filesystem_driver();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (renderer == nullptr) {
        std::printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        deinit_ps2_filesystem_driver();
        return 1;
    }

    const auto rom_path = ps2_frontend::select_rom(renderer);
    if (rom_path.has_value()) {
        std::printf("Selected ROM: %s\n", rom_path->c_str());
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    deinit_ps2_filesystem_driver();
    return rom_path.has_value() ? 0 : 1;
}
