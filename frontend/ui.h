#pragma once

#include "raylib.h"
#include "shared/hardware_constants.h"

#include <string_view>

namespace frontend
{
    class Toast;

    namespace ui
    {
        inline constexpr int ScreenWidth = static_cast<int>(gb_hardware::display::Width);
        inline constexpr int ScreenHeight = static_cast<int>(gb_hardware::display::Height);
        inline constexpr int TopBarHeight = 44;

        enum class Action
        {
            None,
            OpenRom,
            TogglePause,
            Reset,
            ToggleSpeed,
        };

        struct ViewState
        {
            std::string_view status;
            std::string_view rom_path;
            int emulated_fps;
            bool has_gameboy;
            bool paused;
            bool unlimited_speed;
            bool show_run_one_frame_timing;
            double run_one_frame_latest_ms;
            double run_one_frame_average_ms;
        };

        Action draw(const ViewState& state, const Texture2D& texture, const Toast& toast);
    }
}
