# macOS (Cocoa) Support

Official macOS backends for the zero-coupling game core. Each platform-touching module gains a
Cocoa backend behind its **unchanged** `include/zcsr/*.h` contract — `core` / `signal` / `hsm` /
`gamelogic` are pure C and untouched.

## Design — thin C-ABI shim

Cocoa requires Objective-C, but the module logic, structs, and state stay in the existing `.c`
files. Each module's macOS path delegates through a small **C-ABI shim** implemented in a
sibling `*_cocoa.m`:

| Module | C side (`#elif defined(__APPLE__)`) | Objective-C shim (`*_cocoa.m`) |
|--------|--------------------------------------|--------------------------------|
| platform | `zcsr_surface_*` → shim | `platform_cocoa.m` — borderless/transparent/top NSWindow, event pump |
| overlay  | `zcsr_overlay_render` → shim | `overlay_cocoa.m` — dark bg + text + bitmap + ≤3 buttons |
| render   | `zcsr_sprite_batch_flush` → shim | `render_cocoa.m` — solid-rect fill per sprite (placeholder, like X11/Win32) |
| input    | `zcsr_input_pump` → shim + trampoline | `input_cocoa.m` — NSEvent key down/up drain |
| audio    | `backend_play/stop` → shim | `audio_cocoa.m` — NSSound (single-stream) |

The `*_cocoa.h` headers are pure C (same-module, no cross-module/relative includes), so the
zero-coupling guard still holds — verified by `tools/check_no_cross_includes.sh` (now extended to
scan `.c`/`.m`, with the one sanctioned `third_party/` exception).

## Build (on a Mac)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.ncpu)
ctest --test-dir build --output-on-failure
./tools/check_no_cross_includes.sh
```

CMake enables the `OBJC` language only on Apple, globs `modules/*/*.m` into each module, compiles
the `.m` with `-fobjc-arc`, and links `-framework Cocoa` to platform/overlay/render/input/audio.
Linux/Windows builds stay pure C and are unaffected.

## Acceptance (must run on a real Mac — GUI/audio gates)

1. **Build + logic**: `ctest` 10/10, coupling guard OK. (These also pass on Linux.)
2. **Render**: `./build/zcsr_render_bench` → a window appears, 200 sprites draw, best flush `<1ms`.
3. **Overlay**: build & run `verify_overlay_visible` (see project acceptance script) → a dark
   always-on-top window with text, bitmap, and Start/Pause/Stop buttons is visible; `<1ms/frame`.
4. **Input**: keys pressed over the window surface to appear via `zcsr_input_poll` / `is_down`.
5. **Audio**: register a WAV and emit `PLAY_SOUND` / `STOP_SOUND` → sound plays/stops.

## Known caveats (written without a Mac — verify/tune on-device)

- **Not compiled on a Mac yet.** Authored on Linux; needs a macOS toolchain to build & verify.
- **Coordinate flip**: Cocoa origin is bottom-left; zcsr uses top-left. Overlay/render flip `y`
  against the view height — pixel offsets may want tuning on-device.
- **Immediate drawing** via `lockFocus`/`unlockFocus` mirrors the X11/Win32 immediate backends but
  is deprecated on recent macOS; a custom `NSView`+`drawRect:` is the longer-term path.
- **Real RGBA blit**: render draws solid rects (first-pixel color), like the other backends; a
  `CGImage`/`IOSurface` blit is the follow-up.
- **Event-pump ownership**: resolved by design — `zcsr_surface_pump_events` drains all NON-key
  events and leaves key down/up for `zcsr_input_pump` to consume, so a normal "surface pump then
  input pump" loop delivers keys. (Verify on-device that the split behaves as intended.)
- **Close semantics**: borderless windows have no close button; input treats "window no longer
  visible" as a close request.
- **Audio**: NSSound is single-stream (one current sound), matching Win32 `PlaySound`.
