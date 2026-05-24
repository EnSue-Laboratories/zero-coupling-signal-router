/* Agent 3 self-test: hierarchical transitions + persistence into the state buffer. */
#include "zcsr/hsm.h"
#include <stdio.h>
#include <string.h>

static const zcsr_hsm_state states[] = {
    { "idle",          0       },
    { "active",        0       },
    { "active.timing", "active" },
    { "active.paused", "active" },
};
static const zcsr_hsm_transition trans[] = {
    { "idle",          "start",  "active.timing" },
    { "active.timing", "pause",  "active.paused" },
    { "active.paused", "resume", "active.timing" },
    { "active",        "stop",   "idle"          }, /* parent-level: handles 'stop' from any active.* */
};

static void check(const char* label, int cond, int* fails) {
    if (!cond) { printf("FAIL: %s\n", label); (*fails)++; }
}

static const char* persisted(zcsr_state* s) {
    zcsr_value v = zcsr_state_get(s, "hsm.state");
    return (v.type == ZCSR_STR && v.s) ? v.s : "";
}

int main(void) {
    static unsigned char mem[16 * 1024];
    int fails = 0;
    zcsr_state* st = zcsr_state_init(mem, sizeof mem);
    zcsr_hsm* h = zcsr_hsm_create(st, states, 4, trans, 4, "idle");

    check("create", h != 0 && st != 0, &fails);
    check("initial = idle", strcmp(zcsr_hsm_current(h), "idle") == 0, &fails);
    check("persist initial", strcmp(persisted(st), "idle") == 0, &fails);

    check("start -> timing (ret)", zcsr_hsm_dispatch(h, "start", 0), &fails);
    check("now active.timing", strcmp(zcsr_hsm_current(h), "active.timing") == 0, &fails);

    check("pause -> paused (ret)", zcsr_hsm_dispatch(h, "pause", 0), &fails);
    check("now active.paused", strcmp(zcsr_hsm_current(h), "active.paused") == 0, &fails);

    /* HIERARCHICAL: 'stop' is defined on parent 'active', dispatched while in 'active.paused'. */
    check("stop bubbles to parent (ret)", zcsr_hsm_dispatch(h, "stop", 0), &fails);
    check("now idle via parent transition", strcmp(zcsr_hsm_current(h), "idle") == 0, &fails);

    /* unknown signal: no transition, state unchanged */
    check("unknown signal -> false", zcsr_hsm_dispatch(h, "nope", 0) == false, &fails);
    check("still idle", strcmp(zcsr_hsm_current(h), "idle") == 0, &fails);

    check("persist final = idle", strcmp(persisted(st), "idle") == 0, &fails);

    /* persist_to == NULL: HSM works in-memory only; create still succeeds, current() correct. */
    {
        zcsr_hsm* h2 = zcsr_hsm_create(0, states, 4, trans, 4, "idle");
        check("create without persistence", h2 != 0, &fails);
        check("h2 initial idle", h2 && strcmp(zcsr_hsm_current(h2), "idle") == 0, &fails);
        check("h2 start -> timing (in-memory)",
              h2 && zcsr_hsm_dispatch(h2, "start", 0) &&
              strcmp(zcsr_hsm_current(h2), "active.timing") == 0, &fails);
        if (h2) zcsr_hsm_destroy(h2);
    }

    /* Persistence stays consistent across many shrink/grow transitions (relies on create()
     * reserving the hsm.state slot at max size + core's capacity-based reuse). A modest buffer
     * would exhaust here if every grow appended a fresh string. */
    {
        static unsigned char mem2[8 * 1024];
        zcsr_state* st2 = zcsr_state_init(mem2, sizeof mem2);
        zcsr_hsm*   h3  = zcsr_hsm_create(st2, states, 4, trans, 4, "idle");
        const char* cyc[] = { "start", "pause", "resume", "stop" }; /* idle->timing->paused->timing->idle */
        int ok = (h3 != 0);
        for (int r = 0; r < 300 && ok; ++r) {
            for (size_t i = 0; i < 4 && ok; ++i) {
                zcsr_hsm_dispatch(h3, cyc[i], 0);
                if (strcmp(persisted(st2), zcsr_hsm_current(h3)) != 0) ok = 0; /* mirror must track */
            }
        }
        check("hsm.state tracks current() across 300 cycles (no desync/exhaustion)", ok, &fails);
        if (h3) zcsr_hsm_destroy(h3);
    }

    if (!fails) printf("test_hsm: PASS (hierarchical + persistence + in-memory + long-run consistency)\n");
    return fails ? 1 : 0;
}
