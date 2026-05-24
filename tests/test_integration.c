/* Agent 6 — end-to-end integration test: overlay click -> signal router -> HSM -> state buffer.
 * Runs headless: on_pointer is pure hit-test (no display needed); only render() needs a surface. */
#include "zcsr/state_buffer.h"
#include "zcsr/overlay.h"
#include "zcsr/hsm.h"
#include "zcsr/signal.h"
#include <stdio.h>
#include <string.h>

static const zcsr_hsm_state states[] = {
    { "idle", 0 }, { "active", 0 },
    { "active.timing", "active" }, { "active.paused", "active" },
};
static const zcsr_hsm_transition trans[] = {
    { "idle",          "start",  "active.timing" },
    { "active.timing", "pause",  "active.paused" },
    { "active.paused", "resume", "active.timing" },
    { "active",        "stop",   "idle"          },
};

static zcsr_hsm* g = 0;
static bool s_start(const char* p) { return zcsr_hsm_dispatch(g, "start", p); }
static bool s_pause(const char* p) { return zcsr_hsm_dispatch(g, "pause", p); }
static bool s_stop (const char* p) { return zcsr_hsm_dispatch(g, "stop",  p); }
#define CONNS(X) X("start", s_start) X("pause", s_pause) X("stop", s_stop)
ZCSR_DEFINE_ROUTER(it, CONNS)

static const zcsr_button buttons[3] = {
    { "Start", {  10, 40, 60, 24 }, "start" },
    { "Pause", {  80, 40, 60, 24 }, "pause" },
    { "Stop",  { 150, 40, 60, 24 }, "stop"  },
};

static void click(zcsr_overlay* ovl, int x, int y) {
    if (!zcsr_overlay_on_pointer(ovl, x, y, true)) return;
    for (int b = 0; b < 3; ++b) {
        zcsr_rect r = buttons[b].bounds;
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) { it_emit(buttons[b].click_signal, ""); return; }
    }
}

int main(void) {
    static unsigned char mem[16 * 1024];
    int fails = 0;
    zcsr_state*   st  = zcsr_state_init(mem, sizeof mem);
    zcsr_overlay* ovl = zcsr_overlay_create(0); /* headless: no surface; on_pointer still works */
    g = zcsr_hsm_create(st, states, 4, trans, 4, "idle");

#define CK(label, cond) do { if (!(cond)) { printf("FAIL: %s\n", label); fails++; } } while (0)
    CK("setup", st && ovl && g);
    zcsr_overlay_set_buttons(ovl, buttons, 3);
    CK("initial idle", strcmp(zcsr_hsm_current(g), "idle") == 0);

    click(ovl, 40, 52); /* Start */
    CK("Start -> active.timing", strcmp(zcsr_hsm_current(g), "active.timing") == 0);
    CK("state buffer mirrors", strcmp(zcsr_state_get(st, "hsm.state").s, "active.timing") == 0);

    click(ovl, 110, 52); /* Pause */
    CK("Pause -> active.paused", strcmp(zcsr_hsm_current(g), "active.paused") == 0);

    click(ovl, 180, 52); /* Stop — hierarchical via parent 'active' */
    CK("Stop -> idle", strcmp(zcsr_hsm_current(g), "idle") == 0);
    CK("state buffer final idle", strcmp(zcsr_state_get(st, "hsm.state").s, "idle") == 0);

    click(ovl, 300, 100); /* outside any button -> no signal, no transition */
    CK("click outside is a no-op", strcmp(zcsr_hsm_current(g), "idle") == 0);

    if (!fails) printf("test_integration: PASS (overlay click -> signal -> HSM -> state)\n");
    return fails ? 1 : 0;
}
