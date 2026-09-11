#ifndef GBSUNNY_PS2_ROM_SELECTOR_H
#define GBSUNNY_PS2_ROM_SELECTOR_H

#include <optional>
#include <string>

struct SDL_Renderer;

namespace ps2_frontend {

std::optional<std::string> select_rom(SDL_Renderer* renderer);

} // namespace ps2_frontend

#endif
