#ifndef GBSUNNY_PS2_COMPONENT_PERFORMANCE_VISITOR_H
#define GBSUNNY_PS2_COMPONENT_PERFORMANCE_VISITOR_H

#include "profiling/gb_component_visitor.h"

#include <array>
#include <cstdint>

namespace ps2_frontend {

struct ComponentPerformanceSnapshot {
    double cpu_ms = 0.0;
    double ppu_ms = 0.0;
    double timer_ms = 0.0;
    double spu_ms = 0.0;
};

class ComponentPerformanceVisitor final : public profiling::GBComponentVisitor {
public:
    ComponentPerformanceVisitor();

    void begin_frame() override;
    void begin_component(profiling::GBComponent component) override;
    void end_component(profiling::GBComponent component) override;
    void end_frame() override;

    void reset();
    [[nodiscard]] const ComponentPerformanceSnapshot& current() const { return snapshot; }

private:
    static constexpr std::size_t ComponentCount = 4;
    [[nodiscard]] static std::size_t index_of(profiling::GBComponent component);
    [[nodiscard]] double average_ms(std::uint64_t ticks) const;

    std::uint64_t frequency;
    std::uint64_t interval_start;
    std::array<std::uint64_t, ComponentCount> component_start{};
    std::array<std::uint64_t, ComponentCount> total_ticks{};
    unsigned int completed_frames = 0;
    ComponentPerformanceSnapshot snapshot;
};

} // namespace ps2_frontend

#endif
