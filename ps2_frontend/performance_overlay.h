#ifndef GBSUNNY_PS2_PERFORMANCE_OVERLAY_H
#define GBSUNNY_PS2_PERFORMANCE_OVERLAY_H

#include <cstdint>

struct SDL_Renderer;

namespace ps2_frontend {

class PerformanceOverlay {
public:
    PerformanceOverlay();

    void record(std::uint64_t loop_ticks,
                std::uint64_t emulation_ticks,
                std::uint64_t upload_ticks,
                std::uint64_t draw_ticks,
                std::uint64_t present_ticks,
                std::uint64_t audio_ticks,
                int emulated_frames);
    void draw(SDL_Renderer* renderer) const;

private:
    struct Snapshot {
        double emulated_fps = 0.0;
        double frame_ms = 0.0;
        double emulation_ms = 0.0;
        double upload_ms = 0.0;
        double draw_ms = 0.0;
        double present_ms = 0.0;
        double audio_ms = 0.0;
    };

    [[nodiscard]] double average_ms(std::uint64_t ticks, unsigned int samples) const;

    std::uint64_t frequency;
    std::uint64_t interval_start;
    std::uint64_t total_loop_ticks = 0;
    std::uint64_t total_emulation_ticks = 0;
    std::uint64_t total_upload_ticks = 0;
    std::uint64_t total_draw_ticks = 0;
    std::uint64_t total_present_ticks = 0;
    std::uint64_t total_audio_ticks = 0;
    unsigned int rendered_frames = 0;
    unsigned int emulation_work_frames = 0;
    unsigned int completed_emulated_frames = 0;
    Snapshot snapshot;
};

} // namespace ps2_frontend

#endif
