#include "component_performance_visitor.h"

#include <SDL.h>

namespace ps2_frontend {

ComponentPerformanceVisitor::ComponentPerformanceVisitor()
    : frequency(SDL_GetPerformanceFrequency()),
      interval_start(SDL_GetPerformanceCounter())
{
}

void ComponentPerformanceVisitor::begin_frame()
{
}

void ComponentPerformanceVisitor::begin_component(profiling::GBComponent component)
{
    component_start[index_of(component)] = SDL_GetPerformanceCounter();
}

void ComponentPerformanceVisitor::end_component(profiling::GBComponent component)
{
    const std::size_t index = index_of(component);
    total_ticks[index] += SDL_GetPerformanceCounter() - component_start[index];
}

void ComponentPerformanceVisitor::end_frame()
{
    completed_frames++;
    const std::uint64_t now = SDL_GetPerformanceCounter();
    if (now - interval_start < frequency) {
        return;
    }

    snapshot.cpu_ms = average_ms(total_ticks[index_of(profiling::GBComponent::Cpu)]);
    snapshot.ppu_ms = average_ms(total_ticks[index_of(profiling::GBComponent::Ppu)]);
    snapshot.timer_ms = average_ms(total_ticks[index_of(profiling::GBComponent::Timer)]);
    snapshot.spu_ms = average_ms(total_ticks[index_of(profiling::GBComponent::Spu)]);
    reset();
}

void ComponentPerformanceVisitor::reset()
{
    interval_start = SDL_GetPerformanceCounter();
    component_start.fill(0);
    total_ticks.fill(0);
    completed_frames = 0;
}

std::size_t ComponentPerformanceVisitor::index_of(profiling::GBComponent component)
{
    return static_cast<std::size_t>(component);
}

double ComponentPerformanceVisitor::average_ms(std::uint64_t ticks) const
{
    if (completed_frames == 0 || frequency == 0) {
        return 0.0;
    }
    return static_cast<double>(ticks) * 1000.0
        / static_cast<double>(frequency)
        / static_cast<double>(completed_frames);
}

} // namespace ps2_frontend
