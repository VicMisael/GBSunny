#ifndef GBSUNNY_GB_COMPONENT_VISITOR_H
#define GBSUNNY_GB_COMPONENT_VISITOR_H

namespace profiling {

enum class GBComponent {
    Cpu,
    Ppu,
    Timer,
    Spu,
};

class GBComponentObserver {
public:
    virtual ~GBComponentObserver() = default;

    virtual void begin_frame() {}
    virtual void begin_component(GBComponent component) = 0;
    virtual void end_component(GBComponent component) = 0;
    virtual void end_frame() {}
};

} // namespace profiling

#endif
