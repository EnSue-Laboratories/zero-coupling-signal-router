# Task #7 — Classic 2D Game Core (Phase 0: contracts)

Extends the zero-coupling `zcsr` foundation into a minimal 2D game core. Same rules: modules
include **only** `include/zcsr/*` contracts (never each other); no heap (fixed/caller buffers);
`tools/check_no_cross_includes.sh` enforces it. C11.

## Modules ↔ Agents
| Agent | Module             | Contracts | Owns |
|-------|--------------------|-----------|------|
| 1 | `modules/input`     | `clock.h`, `input.h`, `window_event.h` | keyboard down/up, window close + cleanup, 60FPS frame timer |
| 2 | `modules/audio`     | `audio.h` | WAV player in ONE dedicated thread; **signals only** (`PLAY_SOUND`/`STOP_SOUND`) |
| 3 | `modules/render`    | `texture.h`, `sprite.h` | PNG via stb_image (single-header) + resource cache; batch sprite renderer |
| 4 | `modules/gamelogic` | `collision.h`, `save.h` | AABB collision; binary save/load (header + checksum) |
| 5 | integration/tests   | — | update ctests; bench (200 sprites <1ms/frame); zero-coupling guard (Claude) |

Division: **Claude** = these Phase-0 contracts + Agent 5 validation + review. **Codex** = implement Agents 1–4.

## Key invariants
- Audio talks to the rest of the game ONLY through the **signal router** (`signal.h`): wire
  `PLAY_SOUND` → `zcsr_audio_slot_play`, `STOP_SOUND` → `zcsr_audio_slot_stop`. No direct calls
  from game logic into audio.
- Rendering reuses the platform **surface** (`platform.h`) — no new windowing.
- `stb_image.h` is vendored under `third_party/` and included **only** by `modules/render`
  (configured with `STBI_MALLOC` to allocate from the cache buffer, no global heap growth).
- All cross-module communication is via contracts + signals; no module includes another module.

## Bench / acceptance (Agent 5)
- `200 sprites render < 1ms/frame` (gate); renderer should scale toward n*10000 sprites.
- All ctests pass; `check_coupling` green; existing task #6 modules unaffected.

## Phase plan
- **Phase 0 (this):** contracts (`include/zcsr/` headers) + module stub skeleton + build wiring.
- **Phase 1 (Codex, parallel):** Agent 1 input/clock/window, Agent 2 audio, Agent 3 render, Agent 4 gamelogic.
- **Phase 2 (Claude):** integration demo (input → game logic → render + audio-via-signals) + tests + bench.
