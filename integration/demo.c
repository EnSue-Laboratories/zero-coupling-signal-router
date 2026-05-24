/* Agent 6 — Integration & Validation. Owns wiring + demo + benchmark.
 * Includes ONLY contracts — never module sources. Integration depends on interfaces,
 * not implementations, which is what keeps modules zero-coupled. */
#include "zcsr/arena.h"
#include "zcsr/state_buffer.h"
#include "zcsr/platform.h"
#include "zcsr/overlay.h"
#include "zcsr/hsm.h"
#include <stdio.h>

/* Example HSM: idle <-> active.{timing,paused}; 'stop' handled at parent 'active' (hierarchical). */
static const zcsr_hsm_state hsm_states[] = {
    { "idle", 0 }, { "active", 0 },
    { "active.timing", "active" }, { "active.paused", "active" },
};
static const zcsr_hsm_transition hsm_trans[] = {
    { "idle",          "start",  "active.timing" },
    { "active.timing", "pause",  "active.paused" },
    { "active.paused", "resume", "active.timing" },
    { "active",        "stop",   "idle"          },
};

int main(void) {
    /* Fixed, no-heap buffers (well under the 256KB core budget). */
    static unsigned char arena_mem[64 * 1024];
    static unsigned char state_mem[64 * 1024];

    zcsr_arena*   arena = zcsr_arena_init(arena_mem, sizeof arena_mem);
    zcsr_state*   st    = zcsr_state_init(state_mem, sizeof state_mem);
    zcsr_rect     bounds = { 0, 0, 320, 120 };
    zcsr_surface* surf  = zcsr_surface_create("zcsr", bounds);
    zcsr_overlay* ovl   = zcsr_overlay_create(surf);
    zcsr_hsm*     hsm   = zcsr_hsm_create(st, hsm_states, 4, hsm_trans, 4, "idle");

    printf("zcsr demo — module wiring (C11)\n");
    printf("  Agent1 core.arena       : %s\n", arena ? "ready" : "TODO");
    printf("  Agent1 core.state       : %s\n", st    ? "ready" : "TODO");
    printf("  Agent5 platform.surface : %s\n", surf  ? "ready" : "(no DISPLAY / TODO)");
    printf("  Agent4 overlay          : %s\n", ovl   ? "ready" : "TODO");
    printf("  Agent3 hsm              : %s\n", hsm   ? "ready" : "TODO");
    printf("  Agent2 signal           : zcsr_emit() table dispatch (see zcsr/signal.h)\n");

    if (hsm) {
        const char* sigs[] = { "start", "pause", "stop" };
        printf("HSM demo (signal -> state, persisted to state buffer):\n");
        printf("  initial             -> %s\n", zcsr_hsm_current(hsm));
        for (size_t i = 0; i < sizeof sigs / sizeof sigs[0]; ++i) {
            zcsr_value persisted;
            zcsr_hsm_dispatch(hsm, sigs[i], 0);
            persisted = zcsr_state_get(st, "hsm.state");
            printf("  signal '%-6s'      -> %-14s (state buffer: %s)\n",
                   sigs[i], zcsr_hsm_current(hsm), persisted.s ? persisted.s : "");
        }
    }

    /* TODO(Agent 6): once overlay (Agent 4) lands — surface->overlay, set text/bitmap/<=3 buttons,
     * loop: on_pointer(x,y,clicked) -> emit click signal -> hsm dispatch -> state set. */
    return 0;
}
