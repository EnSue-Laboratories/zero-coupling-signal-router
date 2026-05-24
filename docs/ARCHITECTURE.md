# Architecture

Language: **C11/C17**. No heap, no STL (N/A in C), no external dependencies.

## Principle: contracts in, implementations out
All coupling lives in `include/zcsr/` (pure interface headers). A module:
- includes **only** `zcsr/*` contract headers,
- implements the function prototypes declared in its own contract header,
- never includes another module's source.

Contracts use **opaque handles** (`typedef struct zcsr_foo zcsr_foo;`) + C prototypes, so callers
hold a pointer and call functions without seeing the layout. Integration (`integration/`) is the
only place that references every module — through these prototypes, so swapping or stubbing a
module changes nothing elsewhere.

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
caller/arena storage — `zcsr_value` never allocates or frees.

### Core (`arena.h`, `state_buffer.h`) — Agent 1
- `zcsr_arena`: linear/stack allocator placed inside a caller buffer; no `malloc`.
- `zcsr_state`: fixed-capacity key→value store. Reads take `const zcsr_state*`; `zcsr_state_set`
  returns `false` on capacity overflow (never grows). Keep total footprint < 256KB.

### Signal/Slot (`signal.h`) — Agent 2
- Slot = `bool (*)(const char*)`; signal payload is a `const char*`.
- C has no templates, so "zero runtime overhead" = connections live in a `static const zcsr_conn[]`
  table built at compile time (ideally via an X-macro `ZCSR_ROUTER`), with no dynamic registration
  or heap. `zcsr_emit()` is a fixed table walk; type-safety comes from the fixed `zcsr_slot_fn`
  signature. (A working baseline `zcsr_emit` ships in Phase 0; Agent 2 adds the X-macro.)

### HSM (`hsm.h`) — Agent 3
- Instance-owned (no global mutable state). `zcsr_hsm_dispatch(signal, payload)` drives transitions.
- On transition, persist `zcsr_hsm_current()` into the state buffer (e.g. key `hsm.state`).

### Platform (`platform.h`) — Agent 5
- `ZCSR_PLATFORM` selected at compile time via `_WIN32` / `__APPLE__` / `__linux__`.
- `zcsr_surface`: borderless, transparent, always-on-top top-level window. Backends:
  Win32 (layered+topmost), X11 (override-redirect + `_NET_WM_STATE_ABOVE` + ARGB visual),
  Cocoa (borderless `NSWindow`, status-window level). No GL/DX/Vulkan.

### Overlay (`overlay.h`) — Agent 4
- Renders only text + one bitmap + ≤3 buttons via GDI/X11/Cocoa. Target < 1ms/frame.
- `zcsr_overlay_on_pointer`: hover → show item info; click → emit the button's `click_signal`,
  return bool.

## Validation (Agent 6)
- `zcsr_demo`: wiring + spec demo (hover info; click returns bool).
- `zcsr_bench`: 1000 renders → assert `<1ms/frame` and `>=1000 refresh/s` (`<0.1 FPS impact`).
- `check_coupling`: static guard against cross-module includes.
- Cross-platform reality: only **Linux/X11** is verifiable on the build host; Windows/macOS
  need CI runners or follow-up validation (flagged honestly, not assumed).
