/* Agent 6 — Integration & Validation. Owns wiring + demo + benchmark.
 * Includes ONLY contracts — never module sources. Integration depends on interfaces,
 * not implementations, which is what keeps modules zero-coupled. */
#include "zcsr/arena.h"
#include "zcsr/state_buffer.h"
#include "zcsr/platform.h"
#include "zcsr/overlay.h"
#include "zcsr/hsm.h"
#include <stdio.h>

int main(void) {
    /* Fixed, no-heap buffers (well under the 256KB core budget). */
    static unsigned char arena_mem[64 * 1024];
    static unsigned char state_mem[64 * 1024];

    zcsr_arena*   arena = zcsr_arena_init(arena_mem, sizeof arena_mem);
    zcsr_state*   st    = zcsr_state_init(state_mem, sizeof state_mem);
    zcsr_rect     bounds = { 0, 0, 320, 120 };
    zcsr_surface* surf  = zcsr_surface_create("zcsr", bounds);
    zcsr_overlay* ovl   = zcsr_overlay_create(surf);
    zcsr_hsm*     hsm   = zcsr_hsm_create(st);

    printf("zcsr demo — module wiring (Phase 0 skeleton, C11)\n");
    printf("  Agent1 core.arena       : %s\n", arena ? "ready" : "TODO");
    printf("  Agent1 core.state       : %s\n", st    ? "ready" : "TODO");
    printf("  Agent5 platform.surface : %s\n", surf  ? "ready" : "TODO");
    printf("  Agent4 overlay          : %s\n", ovl   ? "ready" : "TODO");
    printf("  Agent3 hsm              : %s\n", hsm   ? "ready" : "TODO");
    printf("  Agent2 signal           : zcsr_emit() table dispatch (see zcsr/signal.h)\n");

    /* TODO(Agent 6): once modules land — surface->overlay, set text/bitmap/<=3 buttons,
     * loop: on_pointer(x,y,clicked) -> emit click signal -> hsm dispatch -> state set.
     * (Hover shows item info; click returns bool only.) */
    return 0;
}
