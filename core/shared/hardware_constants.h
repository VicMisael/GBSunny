#pragma once

#include <cstddef>
#include <cstdint>

namespace gb_hardware {
inline constexpr uint32_t CpuClockHz = 4'194'304;

namespace display {
inline constexpr size_t Width = 160;
inline constexpr size_t Height = 144;
inline constexpr size_t PixelCount = Width * Height;
}

namespace ppu {
inline constexpr uint32_t DotsPerLine = 456;
inline constexpr uint32_t VisibleLines = static_cast<uint32_t>(display::Height);
inline constexpr uint32_t TotalLines = 154;
inline constexpr uint32_t VBlankLines = TotalLines - VisibleLines;
inline constexpr uint32_t DotsPerFrame = DotsPerLine * TotalLines;
inline constexpr uint32_t OamScanDots = 80;
inline constexpr uint32_t DmaCycles = 640;

inline constexpr double FramesPerSecond =
	static_cast<double>(CpuClockHz) / DotsPerFrame;
inline constexpr double FrameSeconds =
	static_cast<double>(DotsPerFrame) / CpuClockHz;
}

namespace apu {
inline constexpr uint32_t SampleRate = 48'000;
inline constexpr uint32_t FrameSequencerHz = 512;
inline constexpr uint32_t FrameSequencerPeriod = CpuClockHz / FrameSequencerHz;
inline constexpr double CyclesPerSample =
	static_cast<double>(CpuClockHz) / SampleRate;
}
}
