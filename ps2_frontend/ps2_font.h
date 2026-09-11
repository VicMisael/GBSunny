#ifndef GBSUNNY_PS2_FONT_H
#define GBSUNNY_PS2_FONT_H

#include <SDL.h>

#include <string_view>

namespace ps2_frontend::font {

void draw_text(
    SDL_Renderer* renderer,
    int x,
    int y,
    std::string_view text,
    SDL_Color color);

} // namespace ps2_frontend::font

#endif
