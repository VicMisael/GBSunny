#include "frontend.h"

#include "audio_ring_buffer.h"
#include "gb.h"
#include "raylib.h"
#include "shared/hardware_constants.h"
#include "utils/emu_flags.h"
#include "utils/file_dialog.h"

#include <algorithm>
#include <atomic>
#include <exception>
#include <memory>
#include <string>
#include <utility>

namespace frontend {
namespace {
constexpr int ScreenWidth = static_cast<int>(gb_hardware::display::Width);
constexpr int ScreenHeight = static_cast<int>(gb_hardware::display::Height);
constexpr int InitialScale = 4;
constexpr int TopBarHeight = 44;
constexpr int AudioBufferFrames = 1024;
constexpr std::size_t AudioRingCapacity = AudioBufferFrames * 8;
constexpr int MaxCatchUpFrames = 6;

using StereoAudioRing = AudioRingBuffer<spu::stereo_sample, AudioRingCapacity>;

std::atomic<StereoAudioRing*> active_audio_ring{ nullptr };

void audio_stream_callback(void* buffer_data, unsigned int frame_count)
{
	auto* output = static_cast<float*>(buffer_data);
	StereoAudioRing* ring = active_audio_ring.load(std::memory_order_acquire);

	for (unsigned int frame = 0; frame < frame_count; ++frame) {
		spu::stereo_sample sample{};
		if (ring != nullptr) {
			ring->pop(sample);
		}

		output[frame * 2] = sample.left;
		output[frame * 2 + 1] = sample.right;
	}
}

struct AppState {
	std::unique_ptr<gb> gameboy;
	std::string rom_path;
	std::string status = "Select a ROM to start";
	StereoAudioRing audio_ring;
	bool paused = false;
	bool unlimited_speed = false;
	bool audio_stream_playing = false;
	int frames_since_stat_update = 0;
	int emulated_fps = 0;
	double last_stat_update = 0.0;
	double emulation_time_accumulator = 0.0;
};

void stop_and_clear_audio(AppState& app, AudioStream& audio_stream)
{
	StopAudioStream(audio_stream);
	app.audio_stream_playing = false;
	app.audio_ring.clear();
}

void toggle_pause(AppState& app, AudioStream& audio_stream)
{
	app.paused = !app.paused;
	stop_and_clear_audio(app, audio_stream);
	app.gameboy->consume_audio_samples();
	app.emulation_time_accumulator = 0.0;
	app.status = app.paused ? "Paused" : "Running";
}

bool ui_button(Rectangle bounds, const char* label)
{
	const Vector2 mouse = GetMousePosition();
	const bool hovered = CheckCollisionPointRec(mouse, bounds);
	const Color fill = hovered ? Color{ 72, 86, 112, 255 } : Color{ 48, 58, 76, 255 };

	DrawRectangleRounded(bounds, 0.14f, 8, fill);
	DrawRectangleRoundedLines(bounds, 0.14f, 8, Color{ 128, 145, 176, 255 });

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

void load_rom(AppState& app, const std::string& path, AudioStream& audio_stream)
{
	try {
		stop_and_clear_audio(app, audio_stream);
		EmuFlags flags;
		flags.useFastPPU = false;
		flags.useNewTimer = false;
		flags.useParallelTicks = true;
		app.gameboy = std::make_unique<gb>(path, flags);
		app.rom_path = path;
		app.paused = false;
		app.unlimited_speed = false;
		app.frames_since_stat_update = 0;
		app.emulated_fps = 0;
		app.last_stat_update = GetTime();
		app.emulation_time_accumulator = 0.0;
		app.status = "Running";
		SetTargetFPS(60);
	}
	catch (const std::exception& error) {
		app.rom_path.clear();
		app.status = std::string("Failed to load ROM: ") + error.what();
	}
}

void open_rom_dialog(AppState& app, AudioStream& audio_stream)
{
	const auto paths = open_file_dialog(
		"Open Game Boy ROM",
		"",
		{ "Game Boy ROMs", "*.gb *.gbc", "All files", "*" });

	if (!paths.empty()) {
		load_rom(app, paths.front(), audio_stream);
	}
}

void draw_top_bar(AppState& app, AudioStream& audio_stream)
{
	DrawRectangle(0, 0, GetScreenWidth(), TopBarHeight, Color{ 24, 28, 36, 255 });

	if (ui_button(Rectangle{ 12, 8, 120, 28 }, "Open ROM")) {
		open_rom_dialog(app, audio_stream);
	}

	if (ui_button(Rectangle{ 144, 8, 84, 28 }, app.paused ? "Resume" : "Pause") && app.gameboy != nullptr) {
		toggle_pause(app, audio_stream);
	}

	if (ui_button(Rectangle{ 240, 8, 74, 28 }, "Reset") && app.gameboy != nullptr) {
		stop_and_clear_audio(app, audio_stream);
		app.gameboy->reset();
		app.status = "Reset";
		app.frames_since_stat_update = 0;
		app.emulated_fps = 0;
		app.last_stat_update = GetTime();
	}

	if (ui_button(Rectangle{ 326, 8, 140, 28 }, app.unlimited_speed ? "Limit Speed" : "Unlimited") && app.gameboy != nullptr) {
		app.unlimited_speed = !app.unlimited_speed;
		stop_and_clear_audio(app, audio_stream);
		app.gameboy->consume_audio_samples();
		app.emulation_time_accumulator = 0.0;
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
	constexpr const char* Title = "GBSunny";
	constexpr const char* Subtitle = "Open a .gb or .gbc ROM to start.";
	constexpr int TitleSize = 32;
	constexpr int SubtitleSize = 18;
	const int center_x = GetScreenWidth() / 2;
	const int center_y = (GetScreenHeight() + TopBarHeight) / 2;

	DrawText(Title, center_x - MeasureText(Title, TitleSize) / 2, center_y - 58, TitleSize, RAYWHITE);
	DrawText(Subtitle, center_x - MeasureText(Subtitle, SubtitleSize) / 2, center_y - 16, SubtitleSize, Color{ 190, 200, 215, 255 });
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
	for (const auto sample : samples) {
		if (!app.audio_ring.push(sample)) {
			break;
		}
	}
}

void discard_audio_samples(AppState& app)
{
	if (app.gameboy != nullptr) {
		app.gameboy->consume_audio_samples();
	}
}

void start_audio_when_ready(AppState& app, AudioStream& audio_stream)
{
	if (app.unlimited_speed || app.audio_stream_playing || app.audio_ring.size() < AudioBufferFrames) {
		return;
	}

	PlayAudioStream(audio_stream);
	app.audio_stream_playing = true;
}
}

int run(std::string initial_rom_path)
{
	AppState app;
	app.rom_path = std::move(initial_rom_path);

	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
	InitWindow(ScreenWidth * InitialScale, ScreenHeight * InitialScale + TopBarHeight, "GBSunny Emulator");
	SetTargetFPS(60);

	InitAudioDevice();
	SetAudioStreamBufferSizeDefault(AudioBufferFrames);
	AudioStream audio_stream = LoadAudioStream(gb_hardware::apu::SampleRate, 32, 2);
	active_audio_ring.store(&app.audio_ring, std::memory_order_release);
	SetAudioStreamCallback(audio_stream, audio_stream_callback);

	Image image = GenImageColor(ScreenWidth, ScreenHeight, BLACK);
	Texture2D texture = LoadTextureFromImage(image);
	UnloadImage(image);
	SetTextureFilter(texture, TEXTURE_FILTER_POINT);

	if (!app.rom_path.empty()) {
		load_rom(app, app.rom_path, audio_stream);
	}

	while (!WindowShouldClose()) {
		const double elapsed = std::min(static_cast<double>(GetFrameTime()), 0.1);
		if (IsKeyPressed(KEY_O)) {
			open_rom_dialog(app, audio_stream);
		}

		if (IsKeyPressed(KEY_SPACE) && app.gameboy != nullptr) {
			toggle_pause(app, audio_stream);
		}

		if (app.gameboy != nullptr && !app.paused) {
			int frames_to_run = 32;
			if (!app.unlimited_speed) {
				app.emulation_time_accumulator += elapsed;
				frames_to_run = std::min(
					static_cast<int>(app.emulation_time_accumulator / gb_hardware::ppu::FrameSeconds),
					MaxCatchUpFrames);
				app.emulation_time_accumulator -= frames_to_run * gb_hardware::ppu::FrameSeconds;
			}
			for (int i = 0; i < frames_to_run; ++i) {
				app.gameboy->run_one_frame();
				app.frames_since_stat_update++;
			}
			if (app.unlimited_speed) {
				discard_audio_samples(app);
			}
			else {
				queue_audio_samples(app);
			}
			const auto& framebuffer = app.gameboy->get_framebuffer();
			UpdateTexture(texture, framebuffer.data());
		}
		start_audio_when_ready(app, audio_stream);

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
		draw_top_bar(app, audio_stream);

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
	StopAudioStream(audio_stream);
	active_audio_ring.store(nullptr, std::memory_order_release);
	UnloadAudioStream(audio_stream);
	CloseAudioDevice();
	CloseWindow();
	return 0;
}
}
