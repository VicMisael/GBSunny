# PS2 Performance Investigations

This document tracks possible performance improvements for the PS2 frontend and
the shared Game Boy emulator core. Items here are investigation candidates, not
confirmed optimizations. Every change should be benchmarked on real PS2 hardware
and checked for emulation regressions before being retained.

## Current baseline

- Game Boy target: approximately 59.73 FPS, or 16.74 ms per emulated frame.
- Real PS2 result: approximately 30 ms of emulation work per Game Boy frame.
- Earlier PCSX2 result: approximately 50 ms of emulation work per frame at the
  default emulated EE clock.
- Increasing PCSX2's EE clock improves performance, indicating that EE CPU time
  is the primary constraint.
- `useFastPPU` and `useNewTimer` are enabled.
- `useDotStepping` is disabled in the PS2 frontend.
- Release builds use `-O3 -DNDEBUG` and the PS2 compiler defaults to R5900,
  N32 ABI, hard-float, and single-float code generation.

At 30 ms per emulated frame, the core reaches at most approximately 33 FPS
before frontend rendering costs. Reaching full speed requires roughly a 1.8x
speedup, or a 44% reduction in emulation time.

## Measurement caveat

The component observer currently brackets CPU, PPU, timer, and SPU execution for
every emulated instruction. Each bracket reads `SDL_GetPerformanceCounter()`
twice. With up to roughly 17,500 instructions and eight counter reads per Game
Boy frame, profiling may perform around 140,000 counter reads per frame.

Consequences:

- Similar 10-15 ms readings for every component may largely represent probe
  overhead.
- Instrumented `EMU` time must not be treated as the uninstrumented baseline.
- Component measurements do not include all observer dispatch and timing costs,
  so their sum does not necessarily equal `EMU`.

Before using the component split to guide optimization:

1. Compare overlay-enabled and overlay-disabled frame times.
2. Measure an empty observer to estimate instrumentation cost.
3. Profile one component per run instead of all four simultaneously.
4. Investigate reading the EE hardware cycle counter directly.
5. Consider sampling one out of every 64 or 256 instructions.
6. Repeat important measurements on real hardware.

## Investigation priorities

### 1. Event-based SPU stepping

`spu::step(cycles)` still calls `tick()` once per Game Boy T-cycle. This means
approximately 70,224 SPU iterations per emulated frame even with dot stepping
disabled.

Investigate advancing directly to the next relevant event:

- 48 kHz output sample
- 512 Hz frame-sequencer event
- Pulse duty transition
- Wave-channel position transition
- Noise LFSR transition
- Sweep, envelope, or length event

Expected benefit: very high. Risk: high, because audio timing and register-write
edge cases must remain accurate.

### 2. Event-based timer stepping

`gb_timer2::step(cycles)` also loops over every T-cycle. Investigate calculating
divider transitions and TIMA falling edges in batches while preserving DIV/TAC
write glitches and delayed TIMA reload behavior.

Expected benefit: high. Risk: medium to high because Game Boy timer edge cases
are timing-sensitive.

### 3. CPU and MMU fast paths

Every opcode fetch passes through the complete MMU path, including region
decoding, DMA checks, boot-ROM checks, a region switch, and often a virtual call.

Investigate:

- A specialized instruction-fetch path
- Permanently bypassing the boot ROM check after `FF50` disables it
- Caching the active cartridge ROM-bank pointer
- Direct ROM, WRAM, and HRAM mappings
- A 256-byte page table with direct pointers or I/O handlers
- Avoiding virtual DMA-state checks for memory regions unaffected by DMA
- Reducing pointer indirection and improving hot-state locality

Expected benefit: high because MMU reads occur several times per instruction.
Risk: medium; memory restrictions and bank switching require careful tests.

### 4. CPU opcode dispatch

The CPU uses nested switches and pointer-to-member tables for several opcode
groups. Indirect member calls may be costly on the R5900 and inhibit inlining.

Investigate and benchmark:

- A generated 256-opcode switch
- Inline ALU and rotation operations
- Predecoded opcode metadata
- Direct handlers versus pointer-to-member dispatch
- Small decoded basic-block caches for straight-line ROM execution

Avoid starting with a JIT. Simpler dispatch and memory improvements should be
measured first.

Expected benefit: medium to high. Risk: medium.

### 5. Tile-oriented scanline rendering

Background and window rendering currently fetch tile-map and tile-row data for
every pixel. Eight adjacent pixels usually share the same tile and tile-row
bytes.

Investigate processing complete tile chunks:

- Fetch one tile-map entry per eight pixels
- Fetch the two tile-row bytes once
- Decode eight pixels together
- Cache decoded tile rows where useful
- Avoid per-pixel division and modulo operations
- Replace general sprite sorting with a fixed-size strategy for at most ten
  visible sprites

Expected benefit: medium to high if PPU time remains significant after removing
profiler distortion. Risk: medium.

### 6. Framebuffer upload and PS2 rendering

Observed texture upload was approximately 8 ms in PCSX2. The current 160x144
RGBA framebuffer transfers roughly 92 KiB per presented frame.

Investigate:

- A 16-bit PS2-native texture format to halve transfer size
- DMA-aligned framebuffer storage
- Persistently locked streaming textures
- Double-buffered texture uploads
- Direct gsKit texture upload instead of `SDL_UpdateTexture()`
- Avoiding redundant uploads when no frame was produced
- Measuring overlay and font-drawing cost separately

Expected benefit: medium. Risk: low to medium, except for a direct gsKit path.

### 7. Audio-buffer allocation

`spu::consume_samples()` swaps the internal vector into a returned temporary.
When that temporary is destroyed, the SPU may lose its capacity and allocate
again during the next frame.

Investigate:

- A persistent ring buffer
- A fixed-capacity buffer sized for approximately 805 stereo samples per frame
- A caller-provided output span
- Direct queueing without returning an owning vector
- Reserving and retaining capacity across frames

Expected benefit: low to medium. Risk: low.

### 8. Link-time and compiler optimization

An isolated LTO Release build succeeded with:

```text
-flto=auto -fno-fat-lto-objects
```

It reduced executable code by approximately 47 KiB, or 1.9%. Investigate its
effect on real-hardware frame time, particularly CPU/MMU inlining.

Also benchmark:

- `-O2` versus `-O3`; smaller code can perform better with limited instruction
  cache.
- LTO enabled and disabled.
- Per-file optimization results for CPU, MMU, PPU, timer, and SPU.

Avoid `-Ofast` or `-ffast-math` until audio correctness has been evaluated.
Stripping debug sections reduces ELF size but should not materially improve
runtime performance because debug sections are not loaded as executable code.

Expected benefit: low to medium. Risk: low for LTO, higher for unsafe math flags.

## Frame scheduling and presentation

The frontend currently uses a wall-clock accumulator and may execute up to three
emulated frames before presenting. This works well on a host that normally runs
faster than real time and only occasionally stalls. When the PS2 is permanently
slower than real time, the accumulator never catches up and effectively becomes
automatic frame skipping.

Example using approximate measurements:

```text
Emulation:   30 ms per Game Boy frame
Presentation: 15 ms

Present every frame:
  30 + 15 = 45 ms, approximately 22 displayed/emulated FPS

Run three, then present:
  (3 * 30) + 15 = 105 ms
  approximately 28.6 emulated FPS and 9.5 displayed FPS
```

The catch-up path improves emulation throughput by amortizing presentation cost,
but increases input latency and visual stutter.

Investigate separate explicit policies:

- Present every completed frame for development and clear measurements.
- Throttle to the Game Boy deadline only when running faster than 59.73 FPS.
- Offer intentional fixed or adaptive frame skipping when running slowly.
- Report emulated FPS and presented FPS separately.
- Measure one-emulated-frame latency independently from the frontend-loop time.

This scheduling work does not make the core faster, but it makes behavior and
performance statistics easier to understand.

## Suggested experiment order

1. Establish uninstrumented real-PS2 and PCSX2 baselines.
2. Quantify observer and overlay overhead.
3. Measure one component at a time using lower-overhead probes.
4. Benchmark one-frame-per-presentation scheduling.
5. Enable LTO and benchmark `-O2` versus `-O3`.
6. Remove repeated audio-vector allocation.
7. Prototype event-based timer stepping with correctness tests.
8. Prototype event-based SPU stepping with audio regression tests.
9. Add an instruction-fetch/MMU fast path.
10. Optimize PPU tile-row rendering.
11. Evaluate 16-bit or direct gsKit framebuffer upload.
12. Re-measure the complete system on real hardware after each isolated change.

## Benchmark record template

Record results using the same ROM, scene, duration, build, and hardware settings:

```text
Date:
Platform: Real PS2 / PCSX2
PCSX2 EE clock:
ROM and scene:
Build preset:
Compiler flags:
Overlay enabled:
Observer enabled:
Dot stepping:
Presentation policy:

Emulated FPS:
Presented FPS:
Average emulation ms:
Average presentation ms:
CPU ms:
PPU ms:
Timer ms:
SPU ms:
Audio underruns or defects:
Video or timing regressions:
Notes:
```

Only compare results gathered with equivalent profiling and presentation modes.
