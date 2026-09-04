#include "ps2_emulator.h"

#include "gb.h"
#include "performance_overlay.h"
#include "ps2_font.h"
#include "shared/hardware_constants.h"

#include <SDL.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

namespace ps2_frontend {
namespace {

constexpr int ScreenWidth = 640;
constexpr int ScreenHeight = 448;
constexpr int GameScale = 3;
constexpr int GameWidth = static_cast<int>(gb_hardware::display::Width) * GameScale;
constexpr int GameHeight = static_cast<int>(gb_hardware::display::Height) * GameScale;
constexpr int GameX = (ScreenWidth - GameWidth) / 2;
constexpr int GameY = 0;
constexpr Uint32 AudioBytesPerSecond = gb_hardware::apu::SampleRate * 2 * sizeof(Sint16);
constexpr Uint32 StartupQueuedAudioBytes = AudioBytesPerSecond / 10;
constexpr Uint32 MaxQueuedAudioBytes = AudioBytesPerSecond / 2;
constexpr unsigned int AudioDebugFrames = 120;
SDL_AudioDeviceID TimedAudioDevice = 0;
Uint64 PreviousAudioQueueTime = 0;

struct AudioRingBuffer {
    std::vector<Sint16> data = std::vector<Sint16>(MaxQueuedAudioBytes / sizeof(Sint16));
    std::size_t read = 0;
    std::size_t write = 0;
    std::size_t count = 0;
};

void audio_callback(void* userdata, Uint8* stream, int length)
{
    auto& buffer = *static_cast<AudioRingBuffer*>(userdata);
    std::memset(stream, 0, static_cast<std::size_t>(length));
    auto* output = reinterpret_cast<Sint16*>(stream);
    const std::size_t requested = static_cast<std::size_t>(length) / sizeof(Sint16);
    const std::size_t supplied = std::min(requested, buffer.count);
    for (std::size_t index = 0; index < supplied; ++index) {
        output[index] = buffer.data[buffer.read];
        buffer.read = (buffer.read + 1) % buffer.data.size();
    }
    buffer.count -= supplied;
}

Uint32 audio_buffer_bytes(SDL_AudioDeviceID device, AudioRingBuffer& buffer)
{
    SDL_LockAudioDevice(device);
    const auto bytes = static_cast<Uint32>(buffer.count * sizeof(Sint16));
    SDL_UnlockAudioDevice(device);
    return bytes;
}

void clear_audio_buffer(SDL_AudioDeviceID device, AudioRingBuffer& buffer)
{
    if (device == 0) return;
    SDL_LockAudioDevice(device);
    buffer.read = 0;
    buffer.write = 0;
    buffer.count = 0;
    SDL_UnlockAudioDevice(device);
}

void reset_audio_timing()
{
    TimedAudioDevice = 0;
    PreviousAudioQueueTime = 0;
}

Sint16 float_to_s16(float sample)
{
    const float clamped = std::clamp(sample, -1.0f, 1.0f);
    if (clamped <= -1.0f) {
        return -32768;
    }
    return static_cast<Sint16>(clamped * 32767.0f);
}

std::vector<Sint16> stretch_audio_samples(
    const std::vector<spu::stereo_sample>& samples,
    std::size_t output_frames)
{
    std::vector<Sint16> pcm;
    if (samples.empty() || output_frames == 0) {
        return pcm;
    }

    pcm.reserve(output_frames * 2);
    const double source_step = output_frames > 1
        ? static_cast<double>(samples.size() - 1) / static_cast<double>(output_frames - 1)
        : 0.0;
    for (std::size_t output_index = 0; output_index < output_frames; ++output_index) {
        const double source_position = output_index * source_step;
        const auto first = static_cast<std::size_t>(source_position);
        const auto second = std::min(first + 1, samples.size() - 1);
        const float fraction = static_cast<float>(source_position - static_cast<double>(first));
        const float left = samples[first].left + (samples[second].left - samples[first].left) * fraction;
        const float right = samples[first].right + (samples[second].right - samples[first].right) * fraction;
        pcm.push_back(float_to_s16(left));
        pcm.push_back(float_to_s16(right));
    }
    return pcm;
}

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

SDL_AudioDeviceID open_audio(AudioRingBuffer& buffer)
{
    SDL_AudioSpec desired{};
    desired.freq = static_cast<int>(gb_hardware::apu::SampleRate);
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = audio_callback;
    desired.userdata = &buffer;

    SDL_AudioSpec obtained{};
    const SDL_AudioDeviceID device = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (device == 0) {
        std::cout << "Audio disabled: " << SDL_GetError() << '\n';
        return 0;
    }
    std::cout << "Audio opened: " << obtained.freq << " Hz, "
              << static_cast<int>(obtained.channels) << " channels, format 0x"
              << std::hex << obtained.format << std::dec << '\n';
    SDL_PauseAudioDevice(device, 0);
    return device;
}

void queue_audio(gb& gameboy, SDL_AudioDeviceID audio_device, AudioRingBuffer& buffer)
{
    const auto samples = gameboy.consume_audio_samples();
    if (audio_device == 0 || samples.empty()) {
        return;
    }

    const Uint64 now = SDL_GetPerformanceCounter();
    const Uint64 frequency = SDL_GetPerformanceFrequency();

    const Uint32 queued_before = audio_buffer_bytes(audio_device, buffer);
    if (queued_before >= MaxQueuedAudioBytes) {
        static unsigned int skipped_log_counter = 0;
        if (++skipped_log_counter == AudioDebugFrames) {
            skipped_log_counter = 0;
            std::cout << "Audio ring full: queued=" << queued_before << '\n';
        }
        return;
    }

    std::size_t output_frames = samples.size();
    if (TimedAudioDevice != audio_device || PreviousAudioQueueTime == 0) {
        TimedAudioDevice = audio_device;
        output_frames = std::max<std::size_t>(
            output_frames,
            StartupQueuedAudioBytes / (2 * sizeof(Sint16)));
    } else {
        const double elapsed_seconds = static_cast<double>(now - PreviousAudioQueueTime)
            / static_cast<double>(frequency);
        const auto real_time_frames = static_cast<std::size_t>(
            elapsed_seconds * static_cast<double>(gb_hardware::apu::SampleRate) + 0.5);
        output_frames = std::max(output_frames, real_time_frames);
    }
    PreviousAudioQueueTime = now;

    const std::size_t available_frames =
        (MaxQueuedAudioBytes - queued_before) / (2 * sizeof(Sint16));
    output_frames = std::min(output_frames, available_frames);
    const auto pcm = stretch_audio_samples(samples, output_frames);
    if (pcm.empty()) {
        return;
    }

    SDL_LockAudioDevice(audio_device);
    const std::size_t writable = std::min(pcm.size(), buffer.data.size() - buffer.count);
    for (std::size_t index = 0; index < writable; ++index) {
        buffer.data[buffer.write] = pcm[index];
        buffer.write = (buffer.write + 1) % buffer.data.size();
    }
    buffer.count += writable;
    SDL_UnlockAudioDevice(audio_device);
    const auto byte_count = static_cast<Uint32>(writable * sizeof(Sint16));

    static unsigned int log_counter = 0;
    if (++log_counter == AudioDebugFrames) {
        log_counter = 0;
        int peak = 0;
        for (const Sint16 sample : pcm) {
            peak = std::max(peak, std::abs(static_cast<int>(sample)));
        }
        std::cout << "Audio queued: samples=" << samples.size()
                  << " output_frames=" << output_frames
                  << " queued_bytes=" << byte_count
                  << " queued=" << audio_buffer_bytes(audio_device, buffer)
                  << " peak=" << peak << '\n';
    }
}

void draw_frame(SDL_Renderer* renderer,
                SDL_Texture* texture,
                bool paused,
                bool show_performance,
                const PerformanceOverlay& performance)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const SDL_Rect destination{GameX, GameY, GameWidth, GameHeight};
    SDL_RenderCopy(renderer, texture, nullptr, &destination);
    if (show_performance) {
        performance.draw(renderer);
    }

    const char* controls = paused
        ? "PAUSED  L1 RESUME  R1 RESET  SQUARE STATS"
        : "L1 PAUSE  R1 RESET  SQUARE STATS  TRIANGLE ROM";
    font::draw_text(renderer, 8, ScreenHeight - 25, controls, {220, 232, 255, 255});
}

} // namespace

EmulatorResult run_emulator(SDL_Renderer* renderer, const std::string& rom_path)
{
    EmuFlags flags;
    flags.useFastPPU = true;
    flags.useNewTimer = true;
    flags.useDotStepping = false;

    auto serial = std::make_shared<serial::ConsoleGBSerial>(std::cout);
    gb gameboy(rom_path, flags, nullptr, std::move(serial));

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
    AudioRingBuffer audio_buffer;
    SDL_AudioDeviceID audio_device = open_audio(audio_buffer);
    bool paused = false;
    bool show_performance = true;
    bool running = true;
    EmulatorResult result = EmulatorResult::Quit;
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
                    clear_audio_buffer(audio_device, audio_buffer);
                    reset_audio_timing();
                } else if (event.key.keysym.sym == SDLK_r) {
                    gameboy.reset();
                    clear_audio_buffer(audio_device, audio_buffer);
                    reset_audio_timing();
                } else if (event.key.keysym.sym == SDLK_F3) {
                    show_performance = !show_performance;
                } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                    result = EmulatorResult::SelectRom;
                }
            } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) {
                    paused = !paused;
                    clear_audio_buffer(audio_device, audio_buffer);
                    reset_audio_timing();
                } else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) {
                    gameboy.reset();
                    clear_audio_buffer(audio_device, audio_buffer);
                    reset_audio_timing();
                } else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_X) {
                    show_performance = !show_performance;
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

        int frames_to_run = 0;
        Uint64 emulation_ticks = 0;
        Uint64 audio_ticks = 0;
        Uint64 upload_ticks = 0;
        if (!paused) {
            const Uint64 emulation_start = SDL_GetPerformanceCounter();
            gameboy.run_one_frame();
            emulation_ticks = SDL_GetPerformanceCounter() - emulation_start;

            const Uint64 audio_start = SDL_GetPerformanceCounter();
            queue_audio(gameboy, audio_device, audio_buffer);
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

        const Uint64 draw_start = SDL_GetPerformanceCounter();
        draw_frame(
            renderer,
            texture,
            paused,
            show_performance,
            performance);
        const Uint64 draw_ticks = SDL_GetPerformanceCounter() - draw_start;

        const Uint64 present_start = SDL_GetPerformanceCounter();
        SDL_RenderPresent(renderer);
        const Uint64 present_ticks = SDL_GetPerformanceCounter() - present_start;
        // The PS2 SDL audio backend feeds audsrv from a worker thread. Give it
        // an explicit scheduling point when emulation keeps the EE saturated.
        SDL_Delay(1);
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
        clear_audio_buffer(audio_device, audio_buffer);
        SDL_CloseAudioDevice(audio_device);
    }
    if (controller != nullptr) {
        SDL_GameControllerClose(controller);
    }
    SDL_DestroyTexture(texture);
    return result;
}

} // namespace ps2_frontend
