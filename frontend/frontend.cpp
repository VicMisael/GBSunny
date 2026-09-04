#include "frontend.h"

#include "audio_ring_buffer.h"
#include "gb.h"
#include "raylib.h"
#include "shared/hardware_constants.h"
#include "toast.h"
#include "ui.h"
#include "utils/debug_utils.h"
#include "utils/emu_flags.h"
#include "utils/file_dialog.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace frontend
{
    namespace
    {
        constexpr int InitialScale = 4;
        constexpr int AudioBufferFrames = 1024;
        constexpr std::size_t AudioRingCapacity = AudioBufferFrames * 8;
        constexpr int MaxCatchUpFrames = 6;
        constexpr std::size_t RunOneFrameTimingSampleCount = 120;

        using StereoAudioRing = AudioRingBuffer<spu::stereo_sample, AudioRingCapacity>;

        std::atomic<StereoAudioRing*> active_audio_ring{nullptr};

        void audio_stream_callback(void* buffer_data, unsigned int frame_count)
        {
            auto* output = static_cast<float*>(buffer_data);
            StereoAudioRing* ring = active_audio_ring.load(std::memory_order_acquire);

            for (unsigned int frame = 0; frame < frame_count; ++frame)
            {
                spu::stereo_sample sample{};
                if (ring != nullptr)
                {
                    ring->pop(sample);
                }

                output[frame * 2] = sample.left;
                output[frame * 2 + 1] = sample.right;
            }
        }

        struct AppState
        {
            std::unique_ptr<gb> gameboy;
            std::string rom_path;
            std::string status = "Select a ROM to start";
            StereoAudioRing audio_ring;
            bool paused = false;
            bool unlimited_speed = false;
            bool audio_stream_playing = false;
            bool run_one_frame_time_show = false;
            int frames_since_stat_update = 0;
            int emulated_fps = 0;
            double last_stat_update = 0.0;
            double emulation_time_accumulator = 0.0;
            std::array<double, RunOneFrameTimingSampleCount> run_one_frame_times_ms{};
            double run_one_frame_time_latest_ms = 0.0;
            double run_one_frame_time_average_ms = 0.0;
            double run_one_frame_time_sum_ms = 0.0;
            std::size_t run_one_frame_time_next_sample = 0;
            std::size_t run_one_frame_time_sample_count = 0;

            Toast toast;
        };

        void reset_run_one_frame_timing(AppState& app)
        {
            app.run_one_frame_times_ms.fill(0.0);
            app.run_one_frame_time_latest_ms = 0.0;
            app.run_one_frame_time_average_ms = 0.0;
            app.run_one_frame_time_sum_ms = 0.0;
            app.run_one_frame_time_next_sample = 0;
            app.run_one_frame_time_sample_count = 0;
        }

        void record_run_one_frame_timing(AppState& app, const double elapsed_ms)
        {
            app.run_one_frame_time_latest_ms = elapsed_ms;

            if (app.run_one_frame_time_sample_count == RunOneFrameTimingSampleCount)
            {
                app.run_one_frame_time_sum_ms -= app.run_one_frame_times_ms[app.run_one_frame_time_next_sample];
            }
            else
            {
                app.run_one_frame_time_sample_count++;
            }

            app.run_one_frame_times_ms[app.run_one_frame_time_next_sample] = elapsed_ms;
            app.run_one_frame_time_sum_ms += elapsed_ms;
            app.run_one_frame_time_next_sample =
                (app.run_one_frame_time_next_sample + 1) % RunOneFrameTimingSampleCount;
            app.run_one_frame_time_average_ms =
                app.run_one_frame_time_sum_ms / static_cast<double>(app.run_one_frame_time_sample_count);
        }

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

        void load_rom(AppState& app, const std::string& path, AudioStream& audio_stream)
        {
            try
            {
                stop_and_clear_audio(app, audio_stream);
                EmuFlags flags;
                flags.useFastPPU = false;
                flags.useNewTimer = true;
                flags.useDotStepping = false;
                auto serial = std::make_shared<serial::ConsoleGBSerial>(std::cout);
                app.gameboy = std::make_unique<gb>(path, flags, nullptr, serial);
                app.gameboy->subscribe<CartridgeLoadedEvent>([](const CartridgeLoadedEvent& event)
                {
                    DebugUtils::breakpoint();
                    std::cout << "Cartridge loaded: " << event.rom_name
                        << " (" << event.rom_type << ") from " << event.rom_path << '\n';
                });

                app.gameboy->subscribe<CartridgeLoadedEvent>([&app](const CartridgeLoadedEvent& event)
                {
                    app.toast.show("Cartridge loaded", event.rom_name + " (" + event.rom_type + ")");
                });

                app.rom_path = path;
                app.paused = false;
                app.unlimited_speed = false;
                app.frames_since_stat_update = 0;
                app.emulated_fps = 0;
                app.last_stat_update = GetTime();
                app.emulation_time_accumulator = 0.0;
                reset_run_one_frame_timing(app);
                app.status = "Running";
                SetTargetFPS(60);
            }
            catch (const std::exception& error)
            {
                app.rom_path.clear();
                app.status = std::string("Failed to load ROM: ") + error.what();
            }
        }

        void open_rom_dialog(AppState& app, AudioStream& audio_stream)
        {
            const auto paths = open_file_dialog(
                "Open Game Boy ROM",
                "",
                {"Game Boy ROMs", "*.gb *.gbc", "All files", "*"});

            if (!paths.empty())
            {
                load_rom(app, paths.front(), audio_stream);
            }
        }

        void handle_ui_action(const ui::Action action, AppState& app, AudioStream& audio_stream)
        {
            switch (action)
            {
            case ui::Action::OpenRom:
                open_rom_dialog(app, audio_stream);
                break;
            case ui::Action::TogglePause:
                toggle_pause(app, audio_stream);
                break;
            case ui::Action::Reset:
                stop_and_clear_audio(app, audio_stream);
                app.gameboy->reset();
                app.status = "Reset";
                app.frames_since_stat_update = 0;
                app.emulated_fps = 0;
                app.last_stat_update = GetTime();
                reset_run_one_frame_timing(app);
                break;
            case ui::Action::ToggleSpeed:
                app.unlimited_speed = !app.unlimited_speed;
                stop_and_clear_audio(app, audio_stream);
                app.gameboy->consume_audio_samples();
                app.emulation_time_accumulator = 0.0;
                app.status = app.unlimited_speed ? "Running unlimited" : "Running";
                app.frames_since_stat_update = 0;
                app.emulated_fps = 0;
                app.last_stat_update = GetTime();
                reset_run_one_frame_timing(app);
                SetTargetFPS(app.unlimited_speed ? 0 : 60);
                break;
            case ui::Action::None:
                break;
            }
        }

        void queue_audio_samples(AppState& app)
        {
            if (app.gameboy == nullptr)
            {
                return;
            }

            auto samples = app.gameboy->consume_audio_samples();
            for (const auto sample : samples)
            {
                if (!app.audio_ring.push(sample))
                {
                    break;
                }
            }
        }

        void discard_audio_samples(AppState& app)
        {
            if (app.gameboy != nullptr)
            {
                app.gameboy->consume_audio_samples();
            }
        }

        void start_audio_when_ready(AppState& app, AudioStream& audio_stream)
        {
            if (app.unlimited_speed || app.audio_stream_playing || app.audio_ring.size() < AudioBufferFrames)
            {
                return;
            }

            PlayAudioStream(audio_stream);
            app.audio_stream_playing = true;
        }

        bool gamepad_button_down(int button)
        {
            return IsGamepadAvailable(0) && IsGamepadButtonDown(0, button);
        }

        void update_joypad(AppState& app)
        {
            if (app.gameboy == nullptr)
            {
                return;
            }

            app.gameboy->set_button(
                JoypadButton::Right,
                IsKeyDown(KEY_RIGHT) || gamepad_button_down(GAMEPAD_BUTTON_LEFT_FACE_RIGHT));
            app.gameboy->set_button(
                JoypadButton::Left,
                IsKeyDown(KEY_LEFT) || gamepad_button_down(GAMEPAD_BUTTON_LEFT_FACE_LEFT));
            app.gameboy->set_button(
                JoypadButton::Up,
                IsKeyDown(KEY_UP) || gamepad_button_down(GAMEPAD_BUTTON_LEFT_FACE_UP));
            app.gameboy->set_button(
                JoypadButton::Down,
                IsKeyDown(KEY_DOWN) || gamepad_button_down(GAMEPAD_BUTTON_LEFT_FACE_DOWN));
            app.gameboy->set_button(
                JoypadButton::A,
                IsKeyDown(KEY_Z) || gamepad_button_down(GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
            app.gameboy->set_button(
                JoypadButton::B,
                IsKeyDown(KEY_X) || gamepad_button_down(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT));
            app.gameboy->set_button(
                JoypadButton::Select,
                IsKeyDown(KEY_RIGHT_SHIFT) || gamepad_button_down(GAMEPAD_BUTTON_MIDDLE_LEFT));
            app.gameboy->set_button(
                JoypadButton::Start,
                IsKeyDown(KEY_ENTER) || gamepad_button_down(GAMEPAD_BUTTON_MIDDLE_RIGHT));
        }
    }

    int run(std::string initial_rom_path)
    {
        AppState app;
        app.rom_path = std::move(initial_rom_path);

        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
        InitWindow(ui::ScreenWidth * InitialScale, ui::ScreenHeight * InitialScale + ui::TopBarHeight,
                   "GBSunny Emulator");
        SetTargetFPS(60);

        InitAudioDevice();
        SetAudioStreamBufferSizeDefault(AudioBufferFrames);
        AudioStream audio_stream = LoadAudioStream(gb_hardware::apu::SampleRate, 32, 2);
        active_audio_ring.store(&app.audio_ring, std::memory_order_release);
        SetAudioStreamCallback(audio_stream, audio_stream_callback);

        Image image = GenImageColor(ui::ScreenWidth, ui::ScreenHeight, BLACK);
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        SetTextureFilter(texture, TEXTURE_FILTER_POINT);

        if (!app.rom_path.empty())
        {
            load_rom(app, app.rom_path, audio_stream);
        }

        while (!WindowShouldClose())
        {
            const double elapsed = std::min(static_cast<double>(GetFrameTime()), 0.1);
            if (IsKeyPressed(KEY_O))
            {
                open_rom_dialog(app, audio_stream);
            }

            if (IsKeyPressed(KEY_SPACE) && app.gameboy != nullptr)
            {
                toggle_pause(app, audio_stream);
            }

            if (IsKeyPressed(KEY_F))
            {
                app.run_one_frame_time_show = !app.run_one_frame_time_show;
                if (app.run_one_frame_time_show)
                {
                    reset_run_one_frame_timing(app);
                }
            }

            update_joypad(app);

            if (app.gameboy != nullptr && !app.paused)
            {
                int frames_to_run = 32;
                if (!app.unlimited_speed)
                {
                    app.emulation_time_accumulator += elapsed;
                    frames_to_run = std::min(
                        static_cast<int>(app.emulation_time_accumulator / gb_hardware::ppu::FrameSeconds),
                        MaxCatchUpFrames);
                    app.emulation_time_accumulator -= frames_to_run * gb_hardware::ppu::FrameSeconds;
                }
                for (int i = 0; i < frames_to_run; ++i)
                {
                    if (app.run_one_frame_time_show)
                    {
                        const auto m_start = std::chrono::steady_clock::now();
                        app.gameboy->run_one_frame();
                        const auto m_end = std::chrono::steady_clock::now();
                        const std::chrono::duration<double, std::milli> elapsed_ms = m_end - m_start;
                        record_run_one_frame_timing(app, elapsed_ms.count());
                    }
                    else
                    {
                        app.gameboy->run_one_frame();
                    }
                    app.frames_since_stat_update++;
                }
                if (app.unlimited_speed)
                {
                    discard_audio_samples(app);
                }
                else
                {
                    queue_audio_samples(app);
                }
                const auto& framebuffer = app.gameboy->get_framebuffer();
                UpdateTexture(texture, framebuffer.data());
            }
            start_audio_when_ready(app, audio_stream);

            const double now = GetTime();
            if (app.last_stat_update == 0.0)
            {
                app.last_stat_update = now;
            }
            if (now - app.last_stat_update >= 1.0)
            {
                app.emulated_fps = app.frames_since_stat_update;
                app.frames_since_stat_update = 0;
                app.last_stat_update = now;
            }

            const ui::Action action = ui::draw(
                {
                    .status = app.status,
                    .rom_path = app.rom_path,
                    .emulated_fps = app.emulated_fps,
                    .has_gameboy = app.gameboy != nullptr,
                    .paused = app.paused,
                    .unlimited_speed = app.unlimited_speed,
                    .show_run_one_frame_timing = app.run_one_frame_time_show,
                    .run_one_frame_latest_ms = app.run_one_frame_time_latest_ms,
                    .run_one_frame_average_ms = app.run_one_frame_time_average_ms
                },
                texture,
                app.toast);
            handle_ui_action(action, app, audio_stream);
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
