#include "performance_overlay.h"

#include "ps2_font.h"

#include <SDL.h>

#include <cstdio>

namespace ps2_frontend {

PerformanceOverlay::PerformanceOverlay()
    : frequency(SDL_GetPerformanceFrequency()),
      interval_start(SDL_GetPerformanceCounter())
{
}

void PerformanceOverlay::record(std::uint64_t loop_ticks,
                                std::uint64_t emulation_ticks,
                                std::uint64_t upload_ticks,
                                std::uint64_t draw_ticks,
                                std::uint64_t present_ticks,
                                std::uint64_t audio_ticks,
                                int emulated_frames)
{
    total_loop_ticks += loop_ticks;
    total_emulation_ticks += emulation_ticks;
    total_upload_ticks += upload_ticks;
    total_draw_ticks += draw_ticks;
    total_present_ticks += present_ticks;
    total_audio_ticks += audio_ticks;
    rendered_frames++;
    emulation_work_frames += emulated_frames > 0 ? 1U : 0U;
    completed_emulated_frames += static_cast<unsigned int>(emulated_frames);

    const std::uint64_t now = SDL_GetPerformanceCounter();
    const std::uint64_t interval_ticks = now - interval_start;
    if (interval_ticks < frequency) {
        return;
    }

    const double seconds = static_cast<double>(interval_ticks) / static_cast<double>(frequency);
    snapshot.emulated_fps = completed_emulated_frames / seconds;
    snapshot.frame_ms = average_ms(total_loop_ticks, rendered_frames);
    snapshot.emulation_ms = average_ms(total_emulation_ticks, completed_emulated_frames);
    snapshot.upload_ms = average_ms(total_upload_ticks, emulation_work_frames);
    snapshot.audio_ms = average_ms(total_audio_ticks, emulation_work_frames);
    snapshot.draw_ms = average_ms(total_draw_ticks, rendered_frames);
    snapshot.present_ms = average_ms(total_present_ticks, rendered_frames);

    interval_start = now;
    total_loop_ticks = 0;
    total_emulation_ticks = 0;
    total_upload_ticks = 0;
    total_draw_ticks = 0;
    total_present_ticks = 0;
    total_audio_ticks = 0;
    rendered_frames = 0;
    emulation_work_frames = 0;
    completed_emulated_frames = 0;
}

void PerformanceOverlay::draw(SDL_Renderer* renderer) const
{
    char first_line[64]{};
    char second_line[64]{};
    std::snprintf(
        first_line,
        sizeof(first_line),
        "FPS %.1F  FRAME %.1FMS  EMU %.1FMS",
        snapshot.emulated_fps,
        snapshot.frame_ms,
        snapshot.emulation_ms);
    std::snprintf(
        second_line,
        sizeof(second_line),
        "UP %.1F  DRAW %.1F  SYNC %.1F  AUD %.1FMS",
        snapshot.upload_ms,
        snapshot.draw_ms,
        snapshot.present_ms,
        snapshot.audio_ms);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 190);
    const SDL_Rect background{4, 4, 632, 34};
    SDL_RenderFillRect(renderer, &background);
    font::draw_text(renderer, 10, 7, first_line, {120, 255, 120, 255});
    font::draw_text(renderer, 10, 21, second_line, {255, 255, 255, 255});
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

double PerformanceOverlay::average_ms(std::uint64_t ticks, unsigned int samples) const
{
    if (samples == 0 || frequency == 0) {
        return 0.0;
    }
    return static_cast<double>(ticks) * 1000.0
        / static_cast<double>(frequency)
        / static_cast<double>(samples);
}

} // namespace ps2_frontend
