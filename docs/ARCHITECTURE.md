# Architecture

## Principle: contracts in, implementations out
All coupling lives in `include/zcsr/` (pure interface headers). A module:
- includes **only** `zcsr/*` contract headers,
- exposes its capability through a **factory** declared in `zcsr/factories.h`,
- never includes another module's source.

Integration (`integration/`) holds the only place that references every module — and it does so
through factories + interfaces, so swapping or stubbing a module changes nothing elsewhere.

## Dependency phases (why not all 6 in parallel from minute 1)
```
Phase 0  (Integration/arch)  contracts + build skeleton                [this commit]
Phase 1  (parallel)          Agent1 core · Agent2 signal · Agent5 platform
Phase 2                      Agent3 hsm (needs core+signal) · Agent4 overlay (needs platform)
Phase 3  (Integration)       wire demo · benchmark · validate · review
```
Phase 1 modules only depend on contracts, so they are genuinely independent once Phase 0 is frozen.

## Contract notes
### Value (`value.h`)
Tagged union of `bool`/`int64`/`const char*` only. Strings are non-owning pointers into
caller/arena storage — `Value` never allocates or frees.

### Core (`arena.h`, `state_buffer.h`) — Agent 1
- `IArena`: linear/stack allocator over a caller buffer; no `malloc`/`new`.
- `IStateReader` / `IStateWriter`: split so most modules see read-only state. `set()` returns
  `false` on capacity overflow (never grows). Keep total footprint < 256KB.

### Signal/Slot (`signal.h`) — Agent 2
- Slot = `bool(*)(const char*)`; signal payload is a `const char*`.
- **Zero runtime overhead** = no dynamic registration, no heap, no runtime lookup table.
  Implement `template <Conn... Cs> struct Router { static void emit(const char* sig, const char* payload); }`
  that resolves matches via a constexpr fold over `Cs`. Keep the Router header-only so calls inline.

### HSM (`hsm.h`) — Agent 3
- Instance-owned (no global mutable state). `dispatch(signal, payload)` drives transitions.
- On transition, persist `current()` into the state buffer (e.g. key `hsm.state`).

### Platform (`platform.h`) — Agent 5
- `kPlatform` selected at compile time via `_WIN32` / `__APPLE__` / `__linux__`.
- `INativeSurface`: borderless, transparent, always-on-top top-level window. Backends:
  Win32 (layered+topmost), X11 (override-redirect + `_NET_WM_STATE_ABOVE` + ARGB visual),
  Cocoa (borderless `NSWindow`, status-window level). No GL/DX/Vulkan.

### Overlay (`overlay.h`) — Agent 4
- Renders only text + one bitmap + ≤3 buttons via GDI/X11/Cocoa. Target < 1ms/frame.
- `onPointer`: hover → show item info; click → emit the button's `clickSignal`, return bool.

## Validation (Agent 6)
- `zcsr_demo`: wiring + spec demo (hover info; click returns bool).
- `zcsr_bench`: 1000 renders → assert `<1ms/frame` and `>=1000 refresh/s` (`<0.1 FPS impact`).
- `check_coupling`: static guard against cross-module includes.
- Cross-platform reality: only **Linux/X11** is verifiable on the build host; Windows/macOS
  need CI runners or follow-up validation (will be flagged honestly, not assumed).
