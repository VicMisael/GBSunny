#ifndef GBSUNNY_PS2_EMULATOR_H
#define GBSUNNY_PS2_EMULATOR_H

#include <string>

struct SDL_Renderer;

namespace ps2_frontend {

enum class EmulatorResult {
    SelectRom,
    Quit,
};

EmulatorResult run_emulator(SDL_Renderer* renderer, const std::string& rom_path);

} // namespace ps2_frontend

#endif
