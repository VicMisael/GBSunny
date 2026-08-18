#include "toast.h"

#include "raylib.h"

#include <algorithm>
#include <utility>

namespace frontend
{
    void Toast::show(std::string new_title, std::string new_message, const double new_duration)
    {
        title = std::move(new_title);
        message = std::move(new_message);
        shown_at = GetTime();
        duration = new_duration;
    }

    void Toast::draw(const float top) const
    {
        const double elapsed = GetTime() - shown_at;
        const double remaining = duration - elapsed;
        if (duration <= 0.0 || remaining <= 0.0)
        {
            return;
        }

        constexpr int TitleFontSize = 18;
        constexpr int MessageFontSize = 14;
        constexpr float Padding = 14.0f;
        constexpr float IconAreaWidth = 34.0f;
        const float content_width = static_cast<float>(std::max(
            MeasureText(title.c_str(), TitleFontSize),
            MeasureText(message.c_str(), MessageFontSize)));
        const float width = content_width + Padding * 2.0f + IconAreaWidth;
        constexpr float Height = 70.0f;

        const float fade_in = std::clamp(static_cast<float>(elapsed / 0.18), 0.0f, 1.0f);
        const float fade_out = std::clamp(static_cast<float>(remaining / 0.35), 0.0f, 1.0f);
        const float opacity = std::min(fade_in, fade_out);
        const float slide = (1.0f - fade_in) * 18.0f;
        const Rectangle bounds{
            static_cast<float>(GetScreenWidth()) - width - 16.0f + slide,
            top,
            width,
            Height,
        };

        const auto faded = [opacity](Color color)
        {
            color.a = static_cast<unsigned char>(static_cast<float>(color.a) * opacity);
            return color;
        };

        Rectangle shadow = bounds;
        shadow.x += 4.0f;
        shadow.y += 5.0f;
        DrawRectangleRounded(shadow, 0.16f, 8, faded(Color{0, 0, 0, 100}));
        DrawRectangleRounded(bounds, 0.16f, 8, faded(Color{38, 46, 60, 245}));
        DrawRectangleRoundedLines(bounds, 0.16f, 8, faded(Color{105, 124, 154, 255}));

        const Vector2 icon_center{bounds.x + Padding + 10.0f, bounds.y + 28.0f};
        DrawCircleV(icon_center, 10.0f, faded(Color{78, 201, 139, 255}));
        DrawLineEx(
            Vector2{icon_center.x - 4.0f, icon_center.y},
            Vector2{icon_center.x - 1.0f, icon_center.y + 3.0f},
            2.0f,
            faded(RAYWHITE));
        DrawLineEx(
            Vector2{icon_center.x - 1.0f, icon_center.y + 3.0f},
            Vector2{icon_center.x + 5.0f, icon_center.y - 4.0f},
            2.0f,
            faded(RAYWHITE));

        const int text_x = static_cast<int>(bounds.x + Padding + IconAreaWidth);
        DrawText(title.c_str(), text_x, static_cast<int>(bounds.y + 12.0f), TitleFontSize, faded(RAYWHITE));
        DrawText(message.c_str(), text_x, static_cast<int>(bounds.y + 39.0f), MessageFontSize,
                 faded(Color{190, 200, 215, 255}));

        const float progress = std::clamp(static_cast<float>(remaining / duration), 0.0f, 1.0f);
        DrawRectangle(
            static_cast<int>(bounds.x),
            static_cast<int>(bounds.y + bounds.height - 3.0f),
            static_cast<int>(bounds.width * progress),
            3,
            faded(Color{78, 201, 139, 255}));
    }
}
