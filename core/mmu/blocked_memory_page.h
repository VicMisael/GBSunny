#pragma once

#include <array>
#include <cstdint>

#include "memory_page.h"

namespace mmu {
    inline constexpr std::array<uint8_t, page_size> blocked_memory_page = [] {
        std::array<uint8_t, page_size> page{};
        page.fill(0xFF);
        return page;
    }();
}
