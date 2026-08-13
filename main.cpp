#include "gb.h"
#include "raylib.h"
#include "utils/file_dialog.h"

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
constexpr int AudioBufferFrames = 1024;
constexpr size_t MaxQueuedAudioFrames = AudioBufferFrames * 4;

struct AppState {
	std::unique_ptr<gb> gameboy;
	std::string rom_path;
	std::string status = "Select a ROM to start";
	std::vector<spu::stereo_sample> pending_audio;
	bool paused = false;
	bool unlimited_speed = false;
	int frames_since_stat_update = 0;
	int emulated_fps = 0;
	double last_stat_update = 0.0;
};

bool ui_button(Rectangle bounds, const char* label)
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
		app.pending_audio.clear();
		app.paused = false;
		app.unlimited_speed = false;
		app.frames_since_stat_update = 0;
		app.emulated_fps = 0;
		app.last_stat_update = GetTime();
		app.status = "Running";
		SetTargetFPS(60);
	}
	catch (const std::exception& error) {
		//app.gameboy.reset();
		app.rom_path.clear();
		app.pending_audio.clear();
		app.status = std::string("Failed to load ROM: ") + error.what();
	}
}

void open_rom_dialog(AppState& app)
{
	const auto paths = open_file_dialog(
		"Open Game Boy ROM",
		"",
		{ "Game Boy ROMs", "*.gb *.gbc", "All files", "*" });

	if (!paths.empty()) {
		load_rom(app, paths.front());
	}
}

void draw_top_bar(AppState& app)
{
	DrawRectangle(0, 0, GetScreenWidth(), TopBarHeight, Color{ 24, 28, 36, 255 });

	if (ui_button(Rectangle{ 12, 8, 120, 28 }, "Open ROM")) {
		open_rom_dialog(app);
	}

	if (ui_button(Rectangle{ 144, 8, 84, 28 }, app.paused ? "Resume" : "Pause") && app.gameboy != nullptr) {
		app.paused = !app.paused;
		app.status = app.paused ? "Paused" : "Running";
	}

	if (ui_button(Rectangle{ 240, 8, 74, 28 }, "Reset") && app.gameboy != nullptr) {
		app.gameboy->reset();
		app.pending_audio.clear();
		app.status = "Reset";
		app.frames_since_stat_update = 0;
		app.emulated_fps = 0;
		app.last_stat_update = GetTime();
	}

	if (ui_button(Rectangle{ 326, 8, 140, 28 }, app.unlimited_speed ? "Limit Speed" : "Unlimited") && app.gameboy != nullptr) {
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

void queue_audio_samples(AppState& app)
{
	if (app.gameboy == nullptr) {
		return;
	}

	auto samples = app.gameboy->consume_audio_samples();
	app.pending_audio.insert(app.pending_audio.end(), samples.begin(), samples.end());

	if (app.pending_audio.size() > MaxQueuedAudioFrames) {
		app.pending_audio.erase(
			app.pending_audio.begin(),
			app.pending_audio.end() - MaxQueuedAudioFrames);
	}
}

void submit_audio_samples(AppState& app, AudioStream& audio_stream)
{
	if (app.pending_audio.empty() || !IsAudioStreamProcessed(audio_stream)) {
		return;
	}

	const size_t frames_to_submit = std::min(app.pending_audio.size(), static_cast<size_t>(AudioBufferFrames));
	UpdateAudioStream(audio_stream, app.pending_audio.data(), static_cast<int>(frames_to_submit));
	app.pending_audio.erase(app.pending_audio.begin(), app.pending_audio.begin() + frames_to_submit);
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

	InitAudioDevice();
	SetAudioStreamBufferSizeDefault(AudioBufferFrames);
	AudioStream audio_stream = LoadAudioStream(spu::sample_rate, 32, 2);
	PlayAudioStream(audio_stream);

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
				queue_audio_samples(app);
				app.frames_since_stat_update++;
			}
			const auto& framebuffer = app.gameboy->get_framebuffer();
			UpdateTexture(texture, framebuffer.data());
		}
		submit_audio_samples(app, audio_stream);

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
	UnloadAudioStream(audio_stream);
	CloseAudioDevice();
	CloseWindow();
	return 0;
}
