#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#include "cartridge/cartridge.h"
#include "cpu/cpu.h"
#include "cpu/cpu_impl2.h"
#include "cpu/icpu.h"
#include "joypad/joypad.h"
#include "logging/core_logger.h"
#include "mmu/MMU.h"
#include "ppu/scanline_ppu/ppu_scanline.h"
#include "serial/gb_serial.h"
#include "shared/interrupt.h"
#include "spu/spu.h"
#include "timer/gb_timer2.h"

namespace {

using Clock = std::chrono::steady_clock;

constexpr uint16_t kProgramOrigin = 0xC000;
constexpr uint16_t kResultOrigin = 0xC100;
constexpr std::size_t kValidationSteps = 200'003;
constexpr std::size_t kWarmupSteps = 100'000;
constexpr std::size_t kSampleCount = 15;
constexpr auto kCalibrationFloor = std::chrono::milliseconds(20);
constexpr auto kTargetSampleTime = std::chrono::milliseconds(50);

volatile uint64_t benchmark_sink = 0;

enum class Implementation {
    Old,
    Impl2,
};

struct Workload {
    std::string_view name;
    std::vector<uint8_t> prefix;
    std::vector<uint8_t> loop;
};

std::vector<uint8_t> assemble(const Workload& workload) {
    std::vector<uint8_t> program = workload.prefix;
    const auto loop_address = static_cast<uint16_t>(kProgramOrigin + program.size());
    program.insert(program.end(), workload.loop.begin(), workload.loop.end());
    program.push_back(0xC3); // JP loop_address
    program.push_back(static_cast<uint8_t>(loop_address & 0xFF));
    program.push_back(static_cast<uint8_t>(loop_address >> 8));
    return program;
}

std::vector<Workload> workloads() {
    return {
        { "dispatch", {}, { 0x00, 0x00, 0x00, 0x00 } },
        { "register-loads",
          { 0x21, 0x00, 0xC1, 0x06, 0x12 },
          { 0x48, 0x51, 0x5A, 0x63, 0x6C, 0x7D, 0x77 } },
        { "alu",
          { 0x21, 0x00, 0xC1, 0x06, 0x13, 0x3E, 0x5A },
          { 0x80, 0x88, 0xA8, 0xB0, 0xB8, 0x90, 0x77 } },
        { "branches",
          { 0x21, 0x00, 0xC1 },
          { 0xAF, 0x20, 0x00, 0x28, 0x00, 0x3C, 0x28, 0x00,
            0x20, 0x00, 0x3D, 0x77 } },
        { "memory",
          { 0x21, 0x00, 0xC1, 0x06, 0x5A },
          { 0x70, 0x34, 0x7E, 0xA8, 0x77 } },
        { "cb-register",
          { 0x21, 0x00, 0xC1, 0x06, 0x81, 0x0E, 0x42, 0x3E, 0xAA },
          { 0xCB, 0x00, 0xCB, 0x11, 0xCB, 0x7C,
            0xCB, 0xAF, 0xCB, 0xF7, 0x77 } },
        { "cb-memory",
          { 0x21, 0x00, 0xC1, 0x36, 0x81 },
          { 0xCB, 0x06, 0xCB, 0x46, 0xCB, 0x86,
            0xCB, 0xC6, 0x7E } },
        { "mixed",
          { 0x21, 0x00, 0xC1, 0x06, 0x13, 0x0E, 0x37, 0x3E, 0x42 },
          { 0x80, 0x89, 0xA8, 0x77, 0x34, 0x7E,
            0xCB, 0x07, 0xCB, 0x40, 0x3C, 0x20, 0x00, 0x05 } },
    };
}

class CpuFixture {
public:
    CpuFixture(Implementation implementation, const Workload& workload) {
        interrupts = std::make_shared<shared::interrupt>();
        ppu = std::make_shared<PPU_scanline>(interrupts);
        timer = std::make_shared<gb_timer2>(interrupts);
        audio = std::make_shared<spu>(interrupts);
        serial = std::make_shared<serial::NullGBSerial>();
        joypad = std::make_shared<Joypad>(interrupts);

        auto info = std::make_unique<CartridgeInfo>();
        info->type = CartridgeType::ROMOnly;
        info->ram_size = RAMSize::None;
        std::vector<uint8_t> rom(0x8000, 0x00);
        rom[0] = 0xC3; // JP kProgramOrigin
        rom[1] = static_cast<uint8_t>(kProgramOrigin & 0xFF);
        rom[2] = static_cast<uint8_t>(kProgramOrigin >> 8);
        cartridge = std::make_shared<NoMBC>(std::move(rom), std::move(info));

        memory = std::make_shared<mmu::MMU>(
            cartridge, ppu, timer, interrupts, audio, serial, joypad,
            std::make_shared<logging::NullCoreLogger>(), false);

        if (implementation == Implementation::Old) {
            processor = std::make_unique<cpu::cpu>(memory, interrupts);
        } else {
            processor = std::make_unique<cpu::CPUImpl2>(memory, interrupts);
        }

        processor->reset();
        memory->write(0xFF50, 1);
        const auto program = assemble(workload);
        for (std::size_t i = 0; i < program.size(); ++i) {
            memory->write(static_cast<uint16_t>(kProgramOrigin + i), program[i]);
        }

        // Execute the cartridge bootstrap jump before starting any measurement.
        processor->step();
    }

    uint64_t run(std::size_t steps) {
        uint64_t cycles = 0;
        for (std::size_t i = 0; i < steps; ++i) {
            cycles += processor->step();
        }
        return cycles;
    }

    std::array<uint8_t, 16> result_signature() const {
        std::array<uint8_t, 16> result{};
        for (std::size_t i = 0; i < result.size(); ++i) {
            result[i] = memory->read(static_cast<uint16_t>(kResultOrigin + i));
        }
        return result;
    }

private:
    std::shared_ptr<shared::interrupt> interrupts;
    std::shared_ptr<PPU_scanline> ppu;
    std::shared_ptr<gb_timer2> timer;
    std::shared_ptr<spu> audio;
    std::shared_ptr<serial::GBSerial> serial;
    std::shared_ptr<Joypad> joypad;
    std::shared_ptr<Cartridge> cartridge;
    std::shared_ptr<mmu::MMU> memory;
    std::unique_ptr<cpu::ICPU> processor;
};

struct ValidationResult {
    uint64_t cycles;
    std::array<uint8_t, 16> signature;
};

ValidationResult validate_one(Implementation implementation, const Workload& workload) {
    CpuFixture fixture(implementation, workload);
    const auto cycles = fixture.run(kValidationSteps);
    return { cycles, fixture.result_signature() };
}

double measure(Implementation implementation, const Workload& workload, std::size_t steps) {
    CpuFixture fixture(implementation, workload);
    fixture.run(kWarmupSteps);
    std::atomic_signal_fence(std::memory_order_seq_cst);
    const auto begin = Clock::now();
    const auto cycles = fixture.run(steps);
    const auto end = Clock::now();
    std::atomic_signal_fence(std::memory_order_seq_cst);
    benchmark_sink = benchmark_sink ^ cycles;
    return std::chrono::duration<double, std::nano>(end - begin).count() /
           static_cast<double>(steps);
}

std::size_t calibrate(const Workload& workload) {
    std::size_t steps = 100'000;
    while (steps < 1'000'000'000) {
        CpuFixture fixture(Implementation::Old, workload);
        fixture.run(kWarmupSteps);
        const auto start = Clock::now();
        fixture.run(steps);
        const auto elapsed = Clock::now() - start;
        if (elapsed >= kCalibrationFloor) {
            const auto scale = std::chrono::duration<double>(kTargetSampleTime).count() /
                               std::chrono::duration<double>(elapsed).count();
            return std::max(steps, static_cast<std::size_t>(steps * scale));
        }
        steps *= 2;
    }
    return steps;
}

double median(std::vector<double> values) {
    std::ranges::sort(values);
    const auto middle = values.size() / 2;
    if (values.size() % 2 == 0) {
        return (values[middle - 1] + values[middle]) / 2.0;
    }
    return values[middle];
}

double median_absolute_deviation(const std::vector<double>& values, double center) {
    std::vector<double> deviations;
    deviations.reserve(values.size());
    for (const double value : values) {
        deviations.push_back(std::abs(value - center));
    }
    return median(std::move(deviations));
}

struct WorkloadResult {
    std::string_view name;
    double old_ns;
    double impl2_ns;
    double old_mad;
    double impl2_mad;
};

} // namespace

int main() {
#ifndef NDEBUG
    std::cerr << "cpu_benchmark must be built in Release mode\n";
    return 2;
#endif

    std::vector<WorkloadResult> results;

    try {
        for (const auto& workload : workloads()) {
            const auto old_validation = validate_one(Implementation::Old, workload);
            const auto impl2_validation = validate_one(Implementation::Impl2, workload);
            if (old_validation.cycles != impl2_validation.cycles ||
                old_validation.signature != impl2_validation.signature) {
                std::cerr << "Correctness mismatch in workload: " << workload.name << '\n';
                return 1;
            }

            const auto steps = calibrate(workload);
            std::vector<double> old_samples;
            std::vector<double> impl2_samples;
            old_samples.reserve(kSampleCount);
            impl2_samples.reserve(kSampleCount);

            for (std::size_t sample = 0; sample < kSampleCount; ++sample) {
                if (sample % 2 == 0) {
                    old_samples.push_back(measure(Implementation::Old, workload, steps));
                    impl2_samples.push_back(measure(Implementation::Impl2, workload, steps));
                } else {
                    impl2_samples.push_back(measure(Implementation::Impl2, workload, steps));
                    old_samples.push_back(measure(Implementation::Old, workload, steps));
                }
            }

            const auto old_center = median(old_samples);
            const auto impl2_center = median(impl2_samples);
            results.push_back({
                workload.name,
                old_center,
                impl2_center,
                median_absolute_deviation(old_samples, old_center),
                median_absolute_deviation(impl2_samples, impl2_center),
            });
        }
    } catch (const std::exception& error) {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        return 1;
    }

    std::cout << std::left << std::setw(18) << "workload"
              << std::right << std::setw(16) << "old ns/step"
              << std::setw(18) << "impl2 ns/step"
              << std::setw(12) << "speedup" << '\n';
    std::cout << std::string(64, '-') << '\n';

    double log_speedup_sum = 0.0;
    for (const auto& result : results) {
        const auto speedup = result.old_ns / result.impl2_ns;
        log_speedup_sum += std::log(speedup);
        std::cout << std::left << std::setw(18) << result.name
                  << std::right << std::fixed << std::setprecision(3)
                  << std::setw(10) << result.old_ns << " +/- "
                  << std::setw(5) << result.old_mad
                  << std::setw(12) << result.impl2_ns << " +/- "
                  << std::setw(5) << result.impl2_mad
                  << std::setw(11) << std::setprecision(3) << speedup << "x\n";
    }

    const auto geometric_mean = std::exp(log_speedup_sum / static_cast<double>(results.size()));
    std::cout << "\nGeometric-mean speedup: " << std::fixed << std::setprecision(3)
              << geometric_mean << "x\n";
    return 0;
}
