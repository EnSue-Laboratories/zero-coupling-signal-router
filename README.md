# Zero-Coupling UI Signal Router

Pure data + signals. **No game logic.** A minimal, dependency-free UI signal router whose
modules are fully decoupled: each compiles independently and talks only through a frozen
interface contract — never by including another module.

> Status: **Phase 0 — interface contracts + buildable skeleton.** Modules are stubs with
> per-agent `TODO`s. Language: **C++17** (freestanding style: no STL, no exceptions/RTTI in modules).

## Why this layout enables zero-coupling
The apparent conflict — *"no cross-module includes"* vs *"all modules compile independently"* —
is resolved by a **shared contract layer** (`include/zcsr/`). Every module includes only those
contract headers; it never includes another module's source. Integration wires modules together
through factory functions (`include/zcsr/factories.h`), so it depends on **interfaces, not
implementations**. The guard `tools/check_no_cross_includes.sh` fails the build if any module
includes another module or a relative path.

## Modules ↔ Agents
| Agent | Module            | Owns                                                            |
|-------|-------------------|-----------------------------------------------------------------|
| 1     | `modules/core`    | stack-only arena + read-only state buffer (string/int/bool, <256KB, no heap) |
| 2     | `modules/signal`  | compile-time, type-safe signal/slot (`void signal(string)` / `bool slot(string)`), zero runtime overhead |
| 3     | `modules/hsm`     | hierarchical state machine; transitions via signals; persists to state buffer |
| 4     | `modules/overlay` | native overlay: text + 1 bitmap + 3 buttons; <1ms/frame         |
| 5     | `modules/platform`| Win32 / X11 / Cocoa, auto-selected at compile time; no GL/DX/Vulkan |
| 6     | `integration/`    | wiring, demo, benchmark, validation (architecture/review owned) |

## Build
```sh
cmake -S . -B build && cmake --build build
./build/zcsr_demo      # prints module wiring status
./build/zcsr_bench      # render benchmark (once overlay lands)
cmake --build build --target check_coupling   # zero-coupling guard
```

## Contracts (the only thing modules may include)
`include/zcsr/`: `value.h`, `arena.h`, `state_buffer.h`, `signal.h`, `hsm.h`, `platform.h`,
`overlay.h`, `factories.h`. See `docs/ARCHITECTURE.md` for the dependency phases and the
signal-router contract details.
