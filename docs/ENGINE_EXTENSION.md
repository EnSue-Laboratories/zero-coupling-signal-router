# Engine Extension — Game-Usable Core

Additive extension of the Zero-Coupling Signal Router + Game Core toward a lean, GPU-capable
SDL2-replacement core. **Strictly additive**: every existing module (overlay, software render,
single-stream audio, keyboard input, core/signal/hsm/platform/gamelogic) stays unchanged. New
modules talk ONLY through their `include/zcsr/*` contracts — no cross-module calls. Each platform
calls native APIs directly. Only new third-party dependency: **glad** (OpenGL 3.3 loader, vendored
under `third_party/glad`, used only by `modules/glrender`). **No runtime heap.** No code-line limit
(the original "line budget" was a spec mistranslation — correctness + zero-coupling first).

## Modules & contracts (Phase 0 = contracts + stubs in this PR; backends follow)

| Agent | Module | Contract | Scope |
|-------|--------|----------|-------|
| 1 | `modules/glrender` | `zcsr/gl_render.h` | GL 3.3 context on the surface; batch sprites (200–10000) w/ pos/rot/scale/tint/alpha; RGBA32 textures; nearest/linear filter; vsync; single FBO render-to-texture; built-in texture + solid shaders; minimal bitmap text. Vendors glad. |
| 2 | `modules/audiomix` | `zcsr/audio_mix.h` | 16-channel mixer; dedicated audio thread + lock-free ring buffer; Win WASAPI / Linux ALSA / macOS AudioUnit; WAV 16-bit 44100 mono/stereo; linear resample only. Coexists with single-stream `audio.h`. |
| 3 | `modules/inputext` | `zcsr/input_ext.h` | Mouse (move/3-button/wheel) + gamepad (≤2, Xbox layout); Win RawInput+XInput / Linux X11+evdev / macOS IOKit HID + GameController. Additive to keyboard `input.h`. |
| 4 | (discipline) | — | No-heap: each module sizes fixed pools to the 200–10000 ceiling (GL CPU staging, audio buffers, input queues); GPU memory left to driver. Threading per-subsystem (NO general pool): audio = 1 dedicated thread; GL = render thread (context is thread-affine); input = main-loop poll. Optional small worker pool only if async asset loading is later needed. |
| 5 | tests/ + integration/ | — | ctest per module (logic) + benches (200–10000 sprite frame time; audio latency ≤25ms; input ≤1 frame). |

## Zero-coupling & validation
- Each module includes ONLY `zcsr/*` contracts (+ its vendored single-header dep). Enforced by
  `tools/check_no_cross_includes.sh` (scans `.c`/`.m`, third_party exempt).
- CMake needs no extra `find_package` beyond system GL/audio/input libs per backend (added when each
  backend lands; Phase-0 stubs need none).
- **Real-hardware acceptance** (headless verifies logic only): GPU needs a display, audio needs a
  sound device, gamepad needs a controller — same gate model as the overlay `<1ms` / Cocoa gates.

## Ownership
Phase 0 (contracts + skeletons + CMake + this doc) = Claude. Agent 1–4 backends = Codex.
Agent 5 (ctests/benches) + reviews = Claude. Modules are independent → buildable in parallel.
Suggested priority: GPU > audio mixer > input.
