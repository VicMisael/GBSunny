#include "ps2_emulator.h"

#include "component_performance_visitor.h"
#include "gb.h"
#include "performance_overlay.h"
#include "ps2_font.h"
#include "shared/hardware_constants.h"

#include <SDL.h>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>

namespace ps2_frontend {
namespace {

constexpr int ScreenWidth = 640;
constexpr int ScreenHeight = 448;
constexpr int GameScale = 3;
constexpr int GameWidth = static_cast<int>(gb_hardware::display::Width) * GameScale;
constexpr int GameHeight = static_cast<int>(gb_hardware::display::Height) * GameScale;
constexpr int GameX = (ScreenWidth - GameWidth) / 2;
constexpr int GameY = 0;
constexpr int MaxCatchUpFrames = 3;
constexpr Uint32 MaxQueuedAudioBytes = 48'000 * 2 * sizeof(float) / 10;

SDL_GameController* open_controller()
{
    for (int index = 0; index < SDL_NumJoysticks(); ++index) {
        if (SDL_IsGameController(index) == SDL_TRUE) {
            return SDL_GameControllerOpen(index);
        }
    }
    return nullptr;
}

bool controller_button(SDL_GameController* controller, SDL_GameControllerButton button)
{
    return controller != nullptr && SDL_GameControllerGetButton(controller, button) != 0;
}

void update_joypad(gb& gameboy, SDL_GameController* controller)
{
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    gameboy.set_button(JoypadButton::Right,
        keys[SDL_SCANCODE_RIGHT] || controller_button(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT));
    gameboy.set_button(JoypadButton::Left,
        keys[SDL_SCANCODE_LEFT] || controller_button(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT));
    gameboy.set_button(JoypadButton::Up,
        keys[SDL_SCANCODE_UP] || controller_button(controller, SDL_CONTROLLER_BUTTON_DPAD_UP));
    gameboy.set_button(JoypadButton::Down,
        keys[SDL_SCANCODE_DOWN] || controller_button(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN));
    gameboy.set_button(JoypadButton::A,
        keys[SDL_SCANCODE_Z] || controller_button(controller, SDL_CONTROLLER_BUTTON_A));
    gameboy.set_button(JoypadButton::B,
        keys[SDL_SCANCODE_X] || controller_button(controller, SDL_CONTROLLER_BUTTON_B));
    gameboy.set_button(JoypadButton::Select,
        keys[SDL_SCANCODE_RSHIFT] || controller_button(controller, SDL_CONTROLLER_BUTTON_BACK));
    gameboy.set_button(JoypadButton::Start,
        keys[SDL_SCANCODE_RETURN] || controller_button(controller, SDL_CONTROLLER_BUTTON_START));
}

SDL_AudioDeviceID open_audio()
{
    SDL_AudioSpec desired{};
    desired.freq = static_cast<int>(gb_hardware::apu::SampleRate);
    desired.format = AUDIO_F32SYS;
    desired.channels = 2;
    desired.samples = 1024;

    const SDL_AudioDeviceID device = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
    if (device == 0) {
        std::cout << "Audio disabled: " << SDL_GetError() << '\n';
        return 0;
    }
    SDL_PauseAudioDevice(device, 0);
    return device;
}

void queue_audio(gb& gameboy, SDL_AudioDeviceID audio_device)
{
    const auto samples = gameboy.consume_audio_samples();
    if (audio_device == 0 || samples.empty()) {
        return;
    }

    if (SDL_GetQueuedAudioSize(audio_device) >= MaxQueuedAudioBytes) {
        return;
    }

    const auto byte_count = static_cast<Uint32>(samples.size() * sizeof(spu::stereo_sample));
    if (SDL_QueueAudio(audio_device, samples.data(), byte_count) != 0) {
        std::cout << "SDL_QueueAudio failed: " << SDL_GetError() << '\n';
    }
}

void draw_frame(SDL_Renderer* renderer,
                SDL_Texture* texture,
                bool paused,
                bool show_performance,
                const PerformanceOverlay& performance,
                const ComponentPerformanceVisitor& component_performance)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const SDL_Rect destination{GameX, GameY, GameWidth, GameHeight};
    SDL_RenderCopy(renderer, texture, nullptr, &destination);
    if (show_performance) {
        performance.draw(renderer, component_performance.current());
    }

    const char* controls = paused
        ? "PAUSED  L1 RESUME  R1 RESET  SQUARE STATS"
        : "L1 PAUSE  R1 RESET  SQUARE STATS  TRIANGLE ROM";
    font::draw_text(renderer, 8, ScreenHeight - 12, controls, {220, 232, 255, 255});
}

} // namespace

EmulatorResult run_emulator(SDL_Renderer* renderer, const std::string& rom_path)
{
    EmuFlags flags;
    flags.useFastPPU = true;
    flags.useNewTimer = true;
    flags.useDotStepping = false;

    ComponentPerformanceVisitor component_performance;
    auto serial = std::make_shared<serial::ConsoleGBSerial>(std::cout);
    gb gameboy(rom_path, flags, nullptr, std::move(serial));
    gameboy.set_component_visitor(&component_performance);
    gameboy.subscribe<CartridgeLoadedEvent>([](const CartridgeLoadedEvent& event) {
        std::cout << "Cartridge loaded: " << event.rom_name
                  << " (" << event.rom_type << ") from " << event.rom_path << '\n';
    });

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        static_cast<int>(gb_hardware::display::Width),
        static_cast<int>(gb_hardware::display::Height));

    if (texture == nullptr) {
        throw std::runtime_error(std::string("SDL_CreateTexture failed: ") + SDL_GetError());
    }
    SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);

    SDL_GameController* controller = open_controller();
    SDL_AudioDeviceID audio_device = open_audio();
    bool paused = false;
    bool show_performance = true;
    bool running = true;
    EmulatorResult result = EmulatorResult::Quit;
    double accumulator = gb_hardware::ppu::FrameSeconds;
    Uint64 previous_counter = SDL_GetPerformanceCounter();
    const double counter_frequency = static_cast<double>(SDL_GetPerformanceFrequency());
    PerformanceOverlay performance;

    while (running) {
        const Uint64 loop_start = SDL_GetPerformanceCounter();
        SDL_Event event{};
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                running = false;
                result = EmulatorResult::Quit;
            } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    paused = !paused;
                    if (audio_device != 0) SDL_ClearQueuedAudio(audio_device);
                } else if (event.key.keysym.sym == SDLK_r) {
                    gameboy.reset();
                    if (audio_device != 0) SDL_ClearQueuedAudio(audio_device);
                } else if (event.key.keysym.sym == SDLK_F3) {
                    show_performance = !show_performance;
                    component_performance.reset();
                    gameboy.set_component_visitor(show_performance ? &component_performance : nullptr);
                } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                    result = EmulatorResult::SelectRom;
                }
            } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) {
                    paused = !paused;
                    if (audio_device != 0) SDL_ClearQueuedAudio(audio_device);
                } else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) {
                    gameboy.reset();
                    if (audio_device != 0) SDL_ClearQueuedAudio(audio_device);
                } else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_X) {
                    show_performance = !show_performance;
                    component_performance.reset();
                    gameboy.set_component_visitor(show_performance ? &component_performance : nullptr);
                } else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_Y) {
                    running = false;
                    result = EmulatorResult::SelectRom;
                }
            }
        }

        if (!running) {
            break;
        }

        update_joypad(gameboy, controller);

        const Uint64 current_counter = SDL_GetPerformanceCounter();
        const double elapsed = std::min(
            static_cast<double>(current_counter - previous_counter) / counter_frequency,
            0.1);
        previous_counter = current_counter;

        int frames_to_run = 0;
        Uint64 emulation_ticks = 0;
        Uint64 audio_ticks = 0;
        Uint64 upload_ticks = 0;
        if (!paused) {
            accumulator += elapsed;
            frames_to_run = std::min(
                static_cast<int>(accumulator / gb_hardware::ppu::FrameSeconds),
                MaxCatchUpFrames);
            accumulator -= frames_to_run * gb_hardware::ppu::FrameSeconds;

            const Uint64 emulation_start = SDL_GetPerformanceCounter();
            for (int frame = 0; frame < frames_to_run; ++frame) {
                gameboy.run_one_frame();
            }
            emulation_ticks = SDL_GetPerformanceCounter() - emulation_start;
            if (frames_to_run > 0) {
                const Uint64 audio_start = SDL_GetPerformanceCounter();
                queue_audio(gameboy, audio_device);
                audio_ticks = SDL_GetPerformanceCounter() - audio_start;

                const auto& framebuffer = gameboy.get_framebuffer();
                const Uint64 upload_start = SDL_GetPerformanceCounter();
                SDL_UpdateTexture(
                    texture,
                    nullptr,
                    framebuffer.data(),
                    static_cast<int>(gb_hardware::display::Width * sizeof(ppu_types::rgba)));
                upload_ticks = SDL_GetPerformanceCounter() - upload_start;
            }
        } else {
            accumulator = 0.0;
        }

        const Uint64 draw_start = SDL_GetPerformanceCounter();
        draw_frame(
            renderer,
            texture,
            paused,
            show_performance,
            performance,
            component_performance);
        const Uint64 draw_ticks = SDL_GetPerformanceCounter() - draw_start;

        const Uint64 present_start = SDL_GetPerformanceCounter();
        SDL_RenderPresent(renderer);
        const Uint64 present_ticks = SDL_GetPerformanceCounter() - present_start;
        const Uint64 loop_ticks = SDL_GetPerformanceCounter() - loop_start;

        performance.record(
            loop_ticks,
            emulation_ticks,
            upload_ticks,
            draw_ticks,
            present_ticks,
            audio_ticks,
            frames_to_run);
    }

    if (audio_device != 0) {
        SDL_ClearQueuedAudio(audio_device);
        SDL_CloseAudioDevice(audio_device);
    }
    if (controller != nullptr) {
        SDL_GameControllerClose(controller);
    }
    SDL_DestroyTexture(texture);
    return result;
}

} // namespace ps2_frontend
