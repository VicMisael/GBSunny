# PS2 Rendering Backend Direction

SDL currently provides a useful proof of concept for the PS2 frontend. It shows
that ROM selection, emulator execution, controller input, audio, framebuffer
presentation, and the performance overlay can work together on the platform.

SDL does not need to remain the final rendering backend. A future implementation
can use gsKit and DMAKit directly while keeping SDL as an optional portable
backend or using it only where it remains beneficial.

## Current path

The current frame path is:

```text
PPU-owned RGBA framebuffer
    -> framebuffer.data()
    -> SDL_UpdateTexture()
    -> SDL_RenderCopy()
    -> SDL_RenderPresent()
```

`gb::get_framebuffer()` returns a reference, and `framebuffer.data()` is already
a pointer to the PPU's existing storage. There is no additional C++ framebuffer
copy when retrieving it.

The expensive operation is the transfer from EE memory into a GS texture. GS
VRAM is separate from normal EE memory, so retaining a pointer alone cannot
provide true zero-copy presentation.

## Architectural goal

The emulator core should not depend on SDL, gsKit, DMAKit, or other PS2-specific
types. It should produce pixels through a small platform-neutral frame-target
interface.

One possible shape is:

```cpp
struct FrameTarget {
    ppu_types::rgba* pixels;
    std::size_t pitch;
};

class VideoBackend {
public:
    virtual ~VideoBackend() = default;

    virtual FrameTarget acquire_frame() = 0;
    virtual void present_frame() = 0;
};
```

The frontend would acquire writable storage before running a frame:

```text
Acquire a writable framebuffer
Point the PPU output at that framebuffer
Run one Game Boy frame
Submit or upload the completed framebuffer
Draw UI and overlays
Present or flip the display buffers
```

The final interface does not have to use virtual functions. A template, callback,
or frontend-owned frame buffer view may be more appropriate after benchmarking.
The important constraint is that the core receives only ordinary pixel memory,
dimensions, and pitch.

## SDL backend

An `SDLVideoBackend` can remain available for development, portability, and as a
reference implementation.

Possible SDL path:

```text
SDL_LockTexture()
    -> expose locked pixels as FrameTarget
    -> run one emulated frame
SDL_UnlockTexture()
SDL_RenderCopy()
SDL_RenderPresent()
```

This can eliminate an intermediate copy into SDL's staging storage if the PPU
renders directly into the locked texture memory. It does not necessarily remove
the EE-to-GS upload performed when the texture is unlocked or presented.

`SDL_LockTexture()` and `SDL_UpdateTexture()` must be benchmarked on real PS2
hardware. The PS2 SDL backend may implement both through similar internal upload
paths, in which case redesigning the core around a locked texture would provide
little benefit.

## Native GS backend

A future `GSVideoBackend` can own the complete native presentation path:

```text
Aligned EE framebuffer
    -> DMA upload to preallocated GS texture memory
    -> draw one textured GS sprite
    -> draw UI or overlay
    -> submit commands and flip buffers
```

Areas to investigate:

- 64-byte or 128-byte-aligned EE framebuffer allocation
- Cached versus uncached EE memory access
- DMAKit transfer requirements and synchronization
- Double or triple EE framebuffer buffering
- Preallocated GS VRAM texture storage
- Persistent texture descriptors
- Nearest-neighbor texture filtering
- Avoiding texture allocation or format conversion per frame
- Uploading only after an emulated frame completes
- Overlapping CPU work with a previous frame's DMA transfer where safe

## Pixel format

The current Game Boy framebuffer uses 32-bit RGBA pixels:

```text
160 * 144 * 4 = 92,160 bytes per frame
```

A 16-bit GS-compatible format would use:

```text
160 * 144 * 2 = 46,080 bytes per frame
```

Using 16-bit pixels halves transfer bandwidth. The Game Boy output uses only a
small grayscale palette, so reduced color precision should not visibly affect
the image.

Options include:

- Keep the core framebuffer in RGBA32 and convert during upload.
- Make the frontend provide a native pixel-conversion target.
- Store palette indices in the core and expand them in the backend.
- Use a small indexed or palette-based upload path if supported efficiently.

Storing palette indices may offer the smallest core framebuffer, but the cost and
complexity of expansion on the EE or GS must be measured.

## Buffer ownership

A direct backend should avoid allocating frame memory during normal execution.
The likely ownership model is:

```text
Video backend owns N aligned EE framebuffers
PPU receives a non-owning view of the current writable buffer
Backend uploads the completed buffer
Backend advances to the next safe writable buffer
```

With double buffering:

```text
Frame N:     CPU writes buffer A, then submits A
Frame N + 1: CPU writes buffer B while A is no longer CPU-owned
Frame N + 2: CPU reuses A after its transfer is complete
```

The backend must not return a buffer still used by an asynchronous DMA transfer.

## UI and overlay composition

Keeping SDL only for UI while drawing the Game Boy framebuffer directly with
gsKit is possible, but it creates an ownership problem. SDL's PS2 renderer may
manage:

- GS state
- Command ordering
- Texture uploads
- Render targets
- Buffer clearing
- VSync and buffer flipping

Issuing direct gsKit commands between SDL renderer calls could invalidate SDL's
state assumptions or cause ordering and synchronization problems.

Cleaner long-term choices are:

### SDL owns the complete frame

SDL draws the Game Boy texture, ROM selector, controls, and profiler. This is the
simplest and most portable option.

### gsKit owns the complete frame

The native backend draws the Game Boy texture and ports the existing bitmap font
and UI rectangles to GS sprites. This gives the frontend complete control over
uploads, command submission, and flipping.

### UI renders into an overlay texture

The UI produces a platform-neutral overlay framebuffer. The native GS backend
uploads and composites the Game Boy texture and UI texture itself. This preserves
some UI portability while keeping GS ownership in one place.

The preferred native design is for gsKit to own the entire frame, either drawing
the UI directly or compositing a frontend-generated overlay texture.

## Proposed backend implementations

```text
VideoBackend
|- SDLVideoBackend
|  |- streaming SDL texture
|  |- SDL renderer composition
|  `- portable/reference implementation
|
`- GSVideoBackend
   |- aligned EE framebuffers
   |- DMAKit texture upload
   |- preallocated GS VRAM
   |- textured-sprite presentation
   `- native UI/overlay composition
```

The emulator core should not know which implementation is active.

## Migration sequence

1. Measure the current `SDL_UpdateTexture()` path on real hardware.
2. Benchmark `SDL_LockTexture()` without changing framebuffer ownership.
3. Introduce a platform-neutral framebuffer view or frame-target abstraction.
4. Allow the PPU to write into frontend-provided storage.
5. Retain the SDL implementation as the reference backend.
6. Prototype an aligned EE framebuffer and direct gsKit texture upload.
7. Compare 32-bit and 16-bit transfer formats.
8. Add double buffering and explicit DMA synchronization.
9. Move the bitmap font and overlay to the native GS composition path.
10. Decide whether SDL remains available as an alternate backend or is removed
    from runtime rendering.

Each step should be independently benchmarked. The SDL path should remain usable
until the GS backend matches its correctness and functionality.

## Validation checklist

A native backend must preserve:

- Correct 160x144 pixel ordering and color mapping
- Nearest-neighbor scaling
- Correct aspect ratio and centering
- Stable presentation without tearing
- No writes into a framebuffer still owned by DMA
- Correct UI and overlay command ordering
- ROM selector functionality
- Pause, reset, and ROM-selection controls
- Equivalent behavior on PCSX2 and real PS2 hardware

Record separately:

- PPU framebuffer production time
- Pixel conversion time
- Texture upload or DMA submission time
- DMA wait time
- GS drawing time
- VSync or flip wait time
- Presented FPS
- Emulated FPS

This separation will show whether a native backend actually reduces frame cost or
merely moves waiting time between stages.
