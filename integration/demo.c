/* Agent 6 — Integration & Validation. Owns wiring + demo + benchmark.
 * Includes ONLY contracts — never module sources. This is where the modules meet:
 *   overlay click -> emit the button's signal (Agent 2 router) -> slot -> HSM dispatch (Agent 3)
 *   -> persist into the core state buffer (Agent 1). Surface/overlay are Agent 5/4. */
#include "zcsr/arena.h"
#include "zcsr/state_buffer.h"
#include "zcsr/platform.h"
#include "zcsr/overlay.h"
#include "zcsr/hsm.h"
#include "zcsr/signal.h"
#include <stdio.h>

/* HSM: idle <-> active.{timing,paused}; 'stop' handled at parent 'active' (hierarchical). */
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

/* Signal router (Agent 2): each UI signal forwards to the HSM. The HSM instance is integration
 * glue, so the (context-free) slots reach it via a file-scope pointer. */
static zcsr_hsm* g_hsm = 0;
static bool to_start(const char* p) { return zcsr_hsm_dispatch(g_hsm, "start", p); }
static bool to_pause(const char* p) { return zcsr_hsm_dispatch(g_hsm, "pause", p); }
static bool to_stop (const char* p) { return zcsr_hsm_dispatch(g_hsm, "stop",  p); }
#define APP_CONNS(X) X("start", to_start) X("pause", to_pause) X("stop", to_stop)
ZCSR_DEFINE_ROUTER(app, APP_CONNS)

/* 3 buttons whose click_signal tags match HSM signals. */
static const zcsr_button buttons[3] = {
    { "Start", {  10, 40, 60, 24 }, "start" },
    { "Pause", {  80, 40, 60, 24 }, "pause" },
    { "Stop",  { 150, 40, 60, 24 }, "stop"  },
};

static int in_rect(zcsr_rect r, int x, int y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

/* The wiring: a click hit-tests the overlay (Agent 4); on a button, integration emits that
 * button's signal through the router (Agent 2). */
static void do_click(zcsr_overlay* ovl, int x, int y) {
    if (!ovl || !zcsr_overlay_on_pointer(ovl, x, y, true)) return;
    for (int b = 0; b < 3; ++b) {
        if (in_rect(buttons[b].bounds, x, y)) { app_emit(buttons[b].click_signal, ""); return; }
    }
}

int main(void) {
    static unsigned char arena_mem[64 * 1024];
    static unsigned char state_mem[64 * 1024];

    zcsr_arena*   arena = zcsr_arena_init(arena_mem, sizeof arena_mem);
    zcsr_state*   st    = zcsr_state_init(state_mem, sizeof state_mem);
    zcsr_rect     bounds = { 0, 0, 320, 120 };
    zcsr_surface* surf  = zcsr_surface_create("zcsr", bounds); /* NULL on a headless host */
    zcsr_overlay* ovl   = zcsr_overlay_create(surf);
    g_hsm = zcsr_hsm_create(st, hsm_states, 4, hsm_trans, 4, "idle");

    printf("zcsr demo — module wiring (C11)\n");
    printf("  Agent1 core            : %s\n", (arena && st) ? "ready" : "TODO");
    printf("  Agent5 platform.surface: %s\n", surf ? "ready" : "(no DISPLAY — logic still runs)");
    printf("  Agent4 overlay         : %s\n", ovl ? "ready" : "TODO");
    printf("  Agent3 hsm             : %s\n", g_hsm ? "ready" : "TODO");
    printf("  Agent2 signal          : compile-time router (app_emit)\n");

    if (ovl) {
        zcsr_overlay_set_text(ovl, "Timer");
        zcsr_overlay_set_buttons(ovl, buttons, 3);
    }

    /* Full loop, runnable even headless (on_pointer is pure hit-test; only render() needs a display). */
    if (g_hsm && ovl) {
        const int clicks[3][2] = { { 40, 52 }, { 110, 52 }, { 180, 52 } }; /* Start / Pause / Stop centers */
        printf("Integration loop (click -> signal -> HSM -> state buffer):\n");
        printf("  initial                 -> %s\n", zcsr_hsm_current(g_hsm));
        for (int c = 0; c < 3; ++c) {
            zcsr_value ps;
            do_click(ovl, clicks[c][0], clicks[c][1]);
            ps = zcsr_state_get(st, "hsm.state");
            printf("  click %-6s          -> %-14s (state buffer: %s)\n",
                   buttons[c].label, zcsr_hsm_current(g_hsm), ps.s ? ps.s : "");
        }
    }
    return 0;
}
