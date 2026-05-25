# Zero-Coupling UI Signal Router

Pure data + signals. A minimal, dependency-light zero-coupling UI/game-core foundation whose
modules compile independently and talk only through shared interface contracts — never by
including another module.

> Status: signal router + overlay + classic 2D game core are implemented; the game-usable engine
> extension is landing additively (OpenGL renderer, audio mixer, universal input). Language:
> **C11/C17** (no runtime heap, no STL; vendored single-file dependencies only where documented).

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
| Agent | Module            | Owns                                                            |
|-------|-------------------|-----------------------------------------------------------------|
| 1     | `modules/core`    | stack-only arena + read-only state buffer (string/int/bool, <256KB, no heap) |
| 2     | `modules/signal`  | signal/slot (`void signal(string)` / `bool slot(string)`); compile-time table, no heap |
| 3     | `modules/hsm`     | hierarchical state machine; transitions via signals; persists to state buffer |
| 4     | `modules/overlay` | native overlay: text + 1 bitmap + 3 buttons; <1ms/frame         |
| 5     | `modules/platform`| Win32 / X11 / Cocoa, auto-selected at compile time; no GL/DX/Vulkan |
| 6     | `integration/`    | wiring, demo, benchmark, validation (architecture/review owned) |

Additional game-core modules live under `modules/input`, `modules/audio`, `modules/render`,
`modules/gamelogic`, `modules/glrender`, `modules/audiomix`, and `modules/inputext`; see
`docs/GAME_CORE.md` and `docs/ENGINE_EXTENSION.md`.

## Build
```sh
cmake -S . -B build && cmake --build build
./build/zcsr_demo      # prints module wiring status
./build/zcsr_bench     # render benchmark (once overlay lands)
cmake --build build --target check_coupling   # zero-coupling guard
```

## Contracts (the only thing modules may include)
`include/zcsr/`: `value.h`, `arena.h`, `state_buffer.h`, `signal.h`, `hsm.h`, `platform.h`,
`overlay.h`. Each module implements the prototypes from its own contract header. See
`docs/ARCHITECTURE.md` for the dependency phases and the signal-router contract details.
