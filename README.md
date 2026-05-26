# Zero-Coupling UI Signal Router

Pure data + signals. A minimal, dependency-light zero-coupling UI/game-core foundation whose
modules compile independently and talk only through shared interface contracts — never by
including another module.

> Status: the signal router, native overlay and classic 2D game core are implemented and accepted
> cross-platform (Win32 / X11 / Cocoa). The **game-usable engine extension has landed additively**:
> OpenGL 3.3 renderer, 16-channel audio mixer, universal input (mouse + gamepad), and an image
> pipeline (decode + offline effects + GPU texture modulation). Language: **C11/C17** — no runtime
> heap, no STL; vendored single-file dependencies only where documented. Real-hardware acceptance
> (GPU timing, sound card, gamepad) is tracked per `docs/ENGINE_EXTENSION.md`.

## Features
A lean, game-usable 2D core aimed at the common SDL2 use cases without the legacy bloat, while
staying zero-coupling and heap-free:

- **Windowing & platform** — Win32 / X11 / Cocoa, auto-selected at compile time (`modules/platform`).
- **Signals & state** — compile-time signal/slot table, hierarchical state machine, stack-only
  arena + read-only state buffer (`modules/core`, `modules/signal`, `modules/hsm`).
- **Native overlay** — text + bitmap + buttons, software-drawn, `<1ms/frame` (`modules/overlay`).
- **Classic game core** — game logic, keyboard input, single-stream WAV audio, software 2D
  render, collision and save (`modules/gamelogic`, `input`, `audio`, `render`).
- **GPU rendering** — optional OpenGL 3.3 sprite renderer with batching + bitmap text
  (`modules/glrender`). Additive: the software/native-2D path stays; GL implements the same sprite
  contract as a drop-in.
- **Audio mixer** — 16-channel WAV mixer over the platform audio backend (`modules/audiomix`).
- **Universal input** — mouse + gamepad layered on top of the existing keyboard input
  (`modules/inputext`; X11/evdev, Win32, Cocoa).
- **Image pipeline** — multi-format decode (PNG / JPEG / BMP / TGA / GIF / PSD / PIC) → RGBA32,
  offline CPU effects (chroma-key / variant textures / palette remap), and runtime GPU texture
  modulation such as combat flash-red and freeze-blue (`modules/image` + GL sprite modulation).

Every capability is **additive** — earlier modules and their accepted behavior are preserved, so
existing integrations keep working byte-for-byte unless they opt into a new path.

## Why this layout enables zero-coupling
The apparent conflict — *"no cross-module includes"* vs *"all modules compile independently"* —
is resolved by a **shared contract layer** (`include/zcsr/`). Every module includes only those
contract headers; it never includes another module's source. Each contract declares an **opaque
handle** (`typedef struct zcsr_foo zcsr_foo;`) plus C function prototypes — the stable ABI.
Integration calls those functions, so it depends on **interfaces, not implementations**. The
guard `tools/check_no_cross_includes.sh` fails the build if any module includes another module
or a relative path.

When a later feature exposes an integration bug inside an earlier module, the repo treats the
feature contract (`.h`) and its implementation (`.c` / same-module native shim) as one cohesive
unit. Small behavior-preserving fixes to an earlier module are allowed when required to complete
the same functional requirement, provided the public contract and accepted behavior are preserved
and the change is reviewed explicitly.

## Modules ↔ Agents
Foundation (signal router):

| Agent | Module            | Owns                                                            |
|-------|-------------------|-----------------------------------------------------------------|
| 1     | `modules/core`    | stack-only arena + read-only state buffer (string/int/bool, <256KB, no heap) |
| 2     | `modules/signal`  | signal/slot (`void signal(string)` / `bool slot(string)`); compile-time table, no heap |
| 3     | `modules/hsm`     | hierarchical state machine; transitions via signals; persists to state buffer |
| 4     | `modules/overlay` | native overlay: text + 1 bitmap + 3 buttons; <1ms/frame         |
| 5     | `modules/platform`| Win32 / X11 / Cocoa, auto-selected at compile time (native 2D; GPU is an additive option — see `glrender`) |
| 6     | `integration/`    | wiring, demo, benchmark, validation (architecture/review owned) |

Classic 2D game core (`docs/GAME_CORE.md`): `modules/gamelogic`, `modules/input`, `modules/audio`,
`modules/render`.

Game-usable engine extension (`docs/ENGINE_EXTENSION.md`):

| Module             | Owns                                                                 |
|--------------------|----------------------------------------------------------------------|
| `modules/glrender` | OpenGL 3.3 sprite renderer: batching, bitmap text, sprite modulation |
| `modules/audiomix` | 16-channel WAV mixer over the platform audio backend                 |
| `modules/inputext` | universal input: mouse + gamepad (X11/evdev, Win32, Cocoa)           |
| `modules/image`    | image decode → RGBA32 + offline chroma-key / variant / palette effects |

## Build
```sh
cmake -S . -B build && cmake --build build
./build/zcsr_demo      # prints module wiring status
./build/zcsr_bench     # render benchmark
ctest --test-dir build # module + integration tests
cmake --build build --target check_coupling   # zero-coupling guard
```

## Contracts (the only thing modules may include)
`include/zcsr/`:
- **Foundation** — `value.h`, `arena.h`, `state_buffer.h`, `signal.h`, `hsm.h`, `platform.h`, `overlay.h`
- **Game core** — `clock.h`, `input.h`, `audio.h`, `sprite.h`, `texture.h`, `collision.h`, `save.h`, `window_event.h`
- **Engine extension** — `gl_render.h`, `audio_mix.h`, `input_ext.h`, `image.h`, `color.h`

Each module implements the prototypes from its own contract header. See `docs/ARCHITECTURE.md`
for the dependency phases, and `docs/GAME_CORE.md` / `docs/ENGINE_EXTENSION.md` for the game core
and extension contracts.
