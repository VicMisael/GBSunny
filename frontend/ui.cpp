#include "ui.h"

#include "toast.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace frontend::ui
{
    namespace
    {
        bool button(const Rectangle bounds, const char* label)
        {
            const Vector2 mouse = GetMousePosition();
            const bool hovered = CheckCollisionPointRec(mouse, bounds);
            const Color fill = hovered ? Color{72, 86, 112, 255} : Color{48, 58, 76, 255};

            DrawRectangleRounded(bounds, 0.14f, 8, fill);
            DrawRectangleRoundedLines(bounds, 0.14f, 8, Color{128, 145, 176, 255});

            constexpr int FontSize = 18;
            const int text_width = MeasureText(label, FontSize);
            DrawText(
                label,
                static_cast<int>(bounds.x + (bounds.width - text_width) / 2.0f),
                static_cast<int>(bounds.y + (bounds.height - FontSize) / 2.0f),
                FontSize,
                RAYWHITE);
            return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        }

        Action draw_top_bar(const ViewState& state)
        {
            DrawRectangle(0, 0, GetScreenWidth(), TopBarHeight, Color{24, 28, 36, 255});

            Action action = Action::None;
            if (button(Rectangle{12, 8, 120, 28}, "Open ROM"))
            {
                action = Action::OpenRom;
            }
            if (button(Rectangle{144, 8, 84, 28}, state.paused ? "Resume" : "Pause") && state.has_gameboy)
            {
                action = Action::TogglePause;
            }
            if (button(Rectangle{240, 8, 74, 28}, "Reset") && state.has_gameboy)
            {
                action = Action::Reset;
            }
            if (button(Rectangle{326, 8, 140, 28}, state.unlimited_speed ? "Limit Speed" : "Unlimited") &&
                state.has_gameboy)
            {
                action = Action::ToggleSpeed;
            }

            const std::string status_text = std::string{state.status} + " | " +
                std::to_string(state.emulated_fps) + " emu fps";
            DrawText(status_text.c_str(), 482, 13, 18, RAYWHITE);
            return action;
        }

        void draw_empty_state()
        {
            constexpr const char* Title = "GBSunny";
            constexpr const char* Subtitle = "Open a .gb or .gbc ROM to start.";
            constexpr int TitleSize = 32;
            constexpr int SubtitleSize = 18;
            const int center_x = GetScreenWidth() / 2;
            const int center_y = (GetScreenHeight() + TopBarHeight) / 2;

            DrawText(Title, center_x - MeasureText(Title, TitleSize) / 2, center_y - 58, TitleSize, RAYWHITE);
            DrawText(Subtitle, center_x - MeasureText(Subtitle, SubtitleSize) / 2, center_y - 16, SubtitleSize,
                     Color{190, 200, 215, 255});
        }

        void draw_screen(const Texture2D& texture)
        {
            const int available_height = GetScreenHeight() - TopBarHeight;
            const int scale_x = GetScreenWidth() / ScreenWidth;
            const int scale_y = available_height / ScreenHeight;
            const int scale = std::max(1, std::min(scale_x, scale_y));
            const auto width = static_cast<float>(ScreenWidth * scale);
            const auto height = static_cast<float>(ScreenHeight * scale);
            const Vector2 position{
                (GetScreenWidth() - width) / 2.0f,
                TopBarHeight + (available_height - height) / 2.0f
            };

            DrawTextureEx(texture, position, 0.0f, static_cast<float>(scale), WHITE);
            DrawRectangleLines(
                static_cast<int>(position.x),
                static_cast<int>(position.y),
                static_cast<int>(width),
                static_cast<int>(height),
                Color{80, 90, 110, 255});
        }

        void draw_frame_timing(const ViewState& state)
        {
            constexpr std::array RegionNames{
                "ROM0", "ROMX", "VRAM", "SRAM", "WRAM0", "WRAMX", "ECHO",
                "OAM", "UNUSED", "IO", "HRAM", "IE", "INVALID"
            };
            constexpr int FontSize = 14;
            constexpr Color TimingColor{255, 220, 120, 255};

            std::ostringstream text;
            text << std::fixed << std::setprecision(3)
                 << "RunOneFrame: " << std::setw(8) << state.run_one_frame_latest_ms << " ms avg "
                 << std::setw(8) << state.run_one_frame_average_ms << " ms";
            DrawText(text.str().c_str(), 12, TopBarHeight + 10, FontSize, TimingColor);

            const auto& stats = state.mmu_read_stats;
            const auto percentage = [&stats](const uint64_t count)
            {
                return stats.total == 0
                    ? 0.0
                    : 100.0 * static_cast<double>(count) / static_cast<double>(stats.total);
            };
            text.str({});
            text.clear();
            text << std::fixed << std::setprecision(2)
                 << "MMU reads: " << stats.total
                 << " slow: " << stats.slow
                 << " (" << percentage(stats.slow) << "%)";
            DrawText(text.str().c_str(), 12, TopBarHeight + 28, FontSize, TimingColor);

            text.str({});
            text.clear();
            text << std::fixed << std::setprecision(2)
                 << "Fast mapped: " << stats.mapped << " (" << percentage(stats.mapped) << "%)"
                 << " HRAM: " << stats.hram << " (" << percentage(stats.hram) << "%)";
            DrawText(text.str().c_str(), 12, TopBarHeight + 46, FontSize, TimingColor);

            text.str({});
            text.clear();
            text << std::fixed << std::setprecision(2)
                 << "DMA blocked: " << stats.dma_blocked
                 << " (" << percentage(stats.dma_blocked) << "%)";
            DrawText(text.str().c_str(), 12, TopBarHeight + 64, FontSize, TimingColor);

            std::string line = "Slow blocks:";
            int line_y = TopBarHeight + 82;
            bool has_regions = false;
            for (std::size_t region = 0; region < stats.slow_by_region.size(); ++region)
            {
                const uint64_t count = stats.slow_by_region[region];
                if (count == 0)
                {
                    continue;
                }

                has_regions = true;
                const std::string entry = " " + std::string{RegionNames[region]} + ":" + std::to_string(count);
                if (MeasureText((line + entry).c_str(), FontSize) > GetScreenWidth() - 24)
                {
                    DrawText(line.c_str(), 12, line_y, FontSize, TimingColor);
                    line = "  " + entry.substr(1);
                    line_y += 18;
                }
                else
                {
                    line += entry;
                }
            }

            if (!has_regions)
            {
                line += " none";
            }
            DrawText(line.c_str(), 12, line_y, FontSize, TimingColor);
        }
    }

    Action draw(const ViewState& state, const Texture2D& texture, const Toast& toast)
    {
        BeginDrawing();
        ClearBackground(Color{12, 14, 18, 255});
        const Action action = draw_top_bar(state);

        if (!state.has_gameboy)
        {
            draw_empty_state();
        }
        else
        {
            draw_screen(texture);
            if (!state.rom_path.empty())
            {
                const std::string rom_path{state.rom_path};
                DrawText(rom_path.c_str(), 12, GetScreenHeight() - 24, 14, Color{150, 160, 176, 255});
            }
            if (state.show_run_one_frame_timing)
            {
                draw_frame_timing(state);
            }
        }

        toast.draw(TopBarHeight + 16.0f);
        EndDrawing();
        return action;
    }
}
