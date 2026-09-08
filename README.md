# GBSunny

A work-in-progress Game Boy emulator, with Game Boy Color support planned for the future.

## Status

- Passes all Blargg instruction tests.
- Includes an SDL desktop frontend and an experimental PS2 frontend.
- The PS2 frontend is CPU-bound on real hardware; see
  [ps2_frontend/PERFORMANCE_OPTIMIZATIONS.md](ps2_frontend/PERFORMANCE_OPTIMIZATIONS.md)
  for the current investigation notes.

## Controls

| Game Boy button | Keyboard | Gamepad |
| --- | --- | --- |
| D-pad Up | Up Arrow | D-pad Up |
| D-pad Down | Down Arrow | D-pad Down |
| D-pad Left | Left Arrow | D-pad Left |
| D-pad Right | Right Arrow | D-pad Right |
| A | Z | Bottom face button |
| B | X | Right face button |
| Select | Right Shift | Back/Select |
| Start | Enter | Start |

### Emulator shortcuts

| Action | Keyboard |
| --- | --- |
| Open ROM | O |
| Pause or resume | Space |

## CPU microbenchmarks

The `cpu_benchmark` target compares the original CPU implementation against
`CPUImpl2` with deterministic synthetic opcode workloads. Results below are
median nanoseconds per CPU `step()`; higher speedup is better.

### Real PS2

| Workload | old ns/step | impl2 ns/step | Speedup |
| --- | ---: | ---: | ---: |
| dispatch | 453.054 | 360.126 | 1.258x |
| register-loads | 482.368 | 485.754 | 0.993x |
| alu | 519.670 | 518.800 | 1.002x |
| branches | 558.010 | 498.860 | 1.119x |
| memory | 599.640 | 572.500 | 1.047x |
| cb-register | 583.820 | 549.430 | 1.063x |
| cb-memory | 707.620 | 657.870 | 1.076x |
| mixed | 565.730 | 538.630 | 1.050x |

Geometric-mean speedup: 1.073x.

### Ryzen 5 3600

| Workload | old ns/step | impl2 ns/step | Speedup |
| --- | ---: | ---: | ---: |
| dispatch | 8.817 | 12.313 | 0.716x |
| register-loads | 11.866 | 10.415 | 1.139x |
| alu | 10.735 | 11.253 | 0.954x |
| branches | 12.148 | 9.725 | 1.249x |
| memory | 12.440 | 11.679 | 1.065x |
| cb-register | 13.044 | 12.099 | 1.078x |
| cb-memory | 14.895 | 12.917 | 1.153x |
| mixed | 12.790 | 11.421 | 1.120x |

Geometric-mean speedup: 1.047x.

Desktop CPUs are dramatically faster than the PS2's R5900, and their branch
prediction, cache, and indirect-call behavior do not always predict real PS2
results. Real-hardware PS2 measurements are the deciding data for PS2-focused
CPU work.

## TODO

- Improve the timer
- Improve the PPU
- Improve halt bug behavior
