#include "gb.h"
#include "portable-file-dialogs.h"
#include "raylib.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <memory>
#include <string>
#include <vector>

namespace {
constexpr int ScreenWidth = 160;
constexpr int ScreenHeight = 144;
constexpr int InitialScale = 4;
constexpr int TopBarHeight = 44;

struct AppState {
	std::unique_ptr<gb> gameboy;
	std::string rom_path;
	std::string status = "Select a ROM to start";
	bool paused = false;
	bool unlimited_speed = false;
	int frames_since_stat_update = 0;
	int emulated_fps = 0;
	double last_stat_update = 0.0;
};

bool button(Rectangle bounds, const char* label)
{
	const Vector2 mouse = GetMousePosition();
	const bool hovered = CheckCollisionPointRec(mouse, bounds);
	const Color fill = hovered ? Color{ 72, 86, 112, 255 } : Color{ 48, 58, 76, 255 };

	DrawRectangleRounded(bounds, 0.14f, 8, fill);
	DrawRectangleRoundedLines(bounds, 0.14f, 8, Color{ 128, 145, 176, 255 });

	const int font_size = 18;
	const int text_width = MeasureText(label, font_size);
	DrawText(
		label,
		static_cast<int>(bounds.x + (bounds.width - text_width) / 2.0f),
		static_cast<int>(bounds.y + (bounds.height - font_size) / 2.0f),
		font_size,
		RAYWHITE);

	return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void load_rom(AppState& app, const std::string& path)
{
	try {
		app.gameboy = std::make_unique<gb>(path, false);
		app.rom_path = path;
		app.paused = false;
		app.unlimited_speed = false;
		app.frames_since_stat_update = 0;
		app.emulated_fps = 0;
		app.last_stat_update = GetTime();
		app.status = "Running";
		SetTargetFPS(60);
	}
	catch (const std::exception& error) {
		app.gameboy.reset();
		app.rom_path.clear();
		app.status = std::string("Failed to load ROM: ") + error.what();
	}
}

void open_rom_dialog(AppState& app)
{
	const auto paths = pfd::open_file(
		"Open Game Boy ROM",
		"",
		{ "Game Boy ROMs", "*.gb *.gbc", "All files", "*" },
		pfd::opt::none).result();

	if (!paths.empty()) {
		load_rom(app, paths.front());
	}
}

void draw_top_bar(AppState& app)
{
	DrawRectangle(0, 0, GetScreenWidth(), TopBarHeight, Color{ 24, 28, 36, 255 });

	if (button(Rectangle{ 12, 8, 120, 28 }, "Open ROM")) {
		open_rom_dialog(app);
	}

	if (button(Rectangle{ 144, 8, 84, 28 }, app.paused ? "Resume" : "Pause") && app.gameboy != nullptr) {
		app.paused = !app.paused;
		app.status = app.paused ? "Paused" : "Running";
	}

	if (button(Rectangle{ 240, 8, 74, 28 }, "Reset") && app.gameboy != nullptr) {
		app.gameboy->reset();
		app.status = "Reset";
		app.frames_since_stat_update = 0;
		app.emulated_fps = 0;
		app.last_stat_update = GetTime();
	}

	if (button(Rectangle{ 326, 8, 140, 28 }, app.unlimited_speed ? "Limit Speed" : "Unlimited") && app.gameboy != nullptr) {
		app.unlimited_speed = !app.unlimited_speed;
		app.status = app.unlimited_speed ? "Running unlimited" : "Running";
		app.frames_since_stat_update = 0;
		app.emulated_fps = 0;
		app.last_stat_update = GetTime();
		SetTargetFPS(app.unlimited_speed ? 0 : 60);
	}

	const std::string status_text = app.status + " | " + std::to_string(app.emulated_fps) + " emu fps";
	DrawText(status_text.c_str(), 482, 13, 18, RAYWHITE);
}

void draw_empty_state()
{
	const char* title = "GBSunny";
	const char* subtitle = "Open a .gb or .gbc ROM to start.";
	const int title_size = 32;
	const int subtitle_size = 18;
	const int center_x = GetScreenWidth() / 2;
	const int center_y = (GetScreenHeight() + TopBarHeight) / 2;

	DrawText(title, center_x - MeasureText(title, title_size) / 2, center_y - 58, title_size, RAYWHITE);
	DrawText(subtitle, center_x - MeasureText(subtitle, subtitle_size) / 2, center_y - 16, subtitle_size, Color{ 190, 200, 215, 255 });
}

void draw_screen(const Texture2D& texture)
{
	const int available_height = GetScreenHeight() - TopBarHeight;
	const int scale_x = GetScreenWidth() / ScreenWidth;
	const int scale_y = available_height / ScreenHeight;
	const int scale = std::max(1, std::min(scale_x, scale_y));
	const float width = static_cast<float>(ScreenWidth * scale);
	const float height = static_cast<float>(ScreenHeight * scale);
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
		Color{ 80, 90, 110, 255 });
}
}

int main(int argc, char* argv[])
{
	AppState app;
	if (argc > 1) {
		app.rom_path = argv[1];
	}

	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
	InitWindow(ScreenWidth * InitialScale, ScreenHeight * InitialScale + TopBarHeight, "GBSunny Emulator");
	SetTargetFPS(60);

	Image image = GenImageColor(ScreenWidth, ScreenHeight, BLACK);
	Texture2D texture = LoadTextureFromImage(image);
	UnloadImage(image);
	SetTextureFilter(texture, TEXTURE_FILTER_POINT);

	if (!app.rom_path.empty()) {
		load_rom(app, app.rom_path);
	}

	while (!WindowShouldClose()) {
		if (IsKeyPressed(KEY_O)) {
			open_rom_dialog(app);
		}

		if (IsKeyPressed(KEY_SPACE) && app.gameboy != nullptr) {
			app.paused = !app.paused;
			app.status = app.paused ? "Paused" : "Running";
		}

		if (app.gameboy != nullptr && !app.paused) {
			const int frames_to_run = app.unlimited_speed ? 32 : 1;
			for (int i = 0; i < frames_to_run; ++i) {
				app.gameboy->run_one_frame();
				app.frames_since_stat_update++;
			}
			const auto& framebuffer = app.gameboy->get_framebuffer();
			UpdateTexture(texture, framebuffer.data());
		}

		const double now = GetTime();
		if (app.last_stat_update == 0.0) {
			app.last_stat_update = now;
		}
		if (now - app.last_stat_update >= 1.0) {
			app.emulated_fps = app.frames_since_stat_update;
			app.frames_since_stat_update = 0;
			app.last_stat_update = now;
		}

		BeginDrawing();
		ClearBackground(Color{ 12, 14, 18, 255 });
		draw_top_bar(app);

		if (app.gameboy == nullptr) {
			draw_empty_state();
		}
		else {
			draw_screen(texture);
			if (!app.rom_path.empty()) {
				DrawText(app.rom_path.c_str(), 12, GetScreenHeight() - 24, 14, Color{ 150, 160, 176, 255 });
			}
		}

		EndDrawing();
	}

	UnloadTexture(texture);
	CloseWindow();
	return 0;
}
