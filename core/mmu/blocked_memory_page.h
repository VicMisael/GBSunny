#pragma once

#include <array>
#include <cstdint>

namespace mmu {
    inline constexpr std::array<uint8_t, 0x100> blocked_memory_page = [] {
        std::array<uint8_t, 0x100> page{};
        page.fill(0xFF);
        return page;
    }();
}
