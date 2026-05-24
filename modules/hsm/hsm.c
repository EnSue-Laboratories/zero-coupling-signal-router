/* Agent 3 — Hierarchical State Machine. No global state, no heap (fixed instance pool).
 * Transitions via signals only; persists current state into the Core state buffer. */
#include "zcsr/hsm.h"

#define ZCSR_HSM_NONE ((size_t)-1)

struct zcsr_hsm {
    bool                       in_use;
    zcsr_state*                persist;
    const zcsr_hsm_state*      states;
    size_t                     state_count;
    const zcsr_hsm_transition* trans;
    size_t                     trans_count;
    size_t                     current; /* index into states */
};

enum { ZCSR_HSM_POOL = 4 };
static struct zcsr_hsm zcsr_hsm_pool[ZCSR_HSM_POOL];

static bool hsm_streq(const char* a, const char* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    while (*a && *b && *a == *b) { ++a; ++b; }
    return *a == *b;
}

static size_t hsm_find_state(const zcsr_hsm* h, const char* name) {
    if (!name || !name[0]) return ZCSR_HSM_NONE;
    for (size_t i = 0; i < h->state_count; ++i) {
        if (hsm_streq(h->states[i].name, name)) return i;
    }
    return ZCSR_HSM_NONE;
}

static void hsm_persist(zcsr_hsm* h) {
    if (h->persist) {
        zcsr_state_set(h->persist, "hsm.state", zcsr_str(zcsr_hsm_current(h)));
    }
}

zcsr_hsm* zcsr_hsm_create(zcsr_state* persist_to,
                          const zcsr_hsm_state* states, size_t state_count,
                          const zcsr_hsm_transition* transitions, size_t transition_count,
                          const char* initial) {
    zcsr_hsm* h = 0;
    size_t init_idx;

    if (!states || state_count == 0 || !initial) return 0;

    for (size_t i = 0; i < ZCSR_HSM_POOL; ++i) {
        if (!zcsr_hsm_pool[i].in_use) { h = &zcsr_hsm_pool[i]; break; }
    }
    if (!h) return 0;

    h->in_use = true;
    h->persist = persist_to;
    h->states = states;
    h->state_count = state_count;
    h->trans = transitions;
    h->trans_count = transition_count;
    h->current = 0;

    init_idx = hsm_find_state(h, initial);
    if (init_idx == ZCSR_HSM_NONE) { h->in_use = false; return 0; }
    h->current = init_idx;
    hsm_persist(h);
    return h;
}

void zcsr_hsm_destroy(zcsr_hsm* h) {
    if (h && h->in_use) h->in_use = false;
}

const char* zcsr_hsm_current(const zcsr_hsm* h) {
    if (!h || !h->in_use || h->current >= h->state_count) return "";
    return h->states[h->current].name ? h->states[h->current].name : "";
}

bool zcsr_hsm_dispatch(zcsr_hsm* h, const char* signal, const char* payload) {
    const char* state_name;
    (void)payload; /* routing is by signal tag; payload is opaque to the HSM */

    if (!h || !h->in_use || !signal) return false;

    /* Hierarchical: try the current state, then bubble up through ancestors. */
    state_name = zcsr_hsm_current(h);
    while (state_name && state_name[0]) {
        size_t si;
        for (size_t t = 0; t < h->trans_count; ++t) {
            if (hsm_streq(h->trans[t].from, state_name) && hsm_streq(h->trans[t].signal, signal)) {
                size_t to_idx = hsm_find_state(h, h->trans[t].to);
                if (to_idx == ZCSR_HSM_NONE) return false; /* invalid target */
                h->current = to_idx;
                hsm_persist(h);
                return true;
            }
        }
        si = hsm_find_state(h, state_name);
        if (si == ZCSR_HSM_NONE) break;
        state_name = h->states[si].parent; /* NULL/"" stops the walk */
    }
    return false;
}
