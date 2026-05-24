#ifndef ZCSR_HSM_H
#define ZCSR_HSM_H
/* Agent 3 — Hierarchical State Machine. No global state, no heap. Transitions via signals only.
 * Persists current state to the Core state buffer (key "hsm.state"). Implementer: modules/hsm.
 *
 * Config is caller-owned const tables (compile-time, zero-coupling): states form a tree via
 * `parent` names; a transition fires when the current state — or any ANCESTOR of it — has a
 * matching (from, signal) entry (that's the "hierarchical" part). */
#include "state_buffer.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct zcsr_hsm zcsr_hsm; /* opaque; instance-owned */

/* A state node. `parent` names the enclosing state (NULL or "" = top-level). */
typedef struct {
    const char* name;
    const char* parent;
} zcsr_hsm_state;

/* When in `from` (or any descendant of it) and `signal` arrives, transition to `to`. */
typedef struct {
    const char* from;
    const char* signal;
    const char* to;
} zcsr_hsm_transition;

/* Build an HSM over caller-owned const tables (no heap; instance from a fixed pool).
 * Persists the initial state into `persist_to`. Returns NULL on bad config / pool exhausted. */
zcsr_hsm*   zcsr_hsm_create(zcsr_state* persist_to,
                            const zcsr_hsm_state* states, size_t state_count,
                            const zcsr_hsm_transition* transitions, size_t transition_count,
                            const char* initial);
void        zcsr_hsm_destroy(zcsr_hsm*);
/* Deliver a signal; walks current state up its ancestors for a matching transition.
 * Returns true if a transition occurred (and persists the new current state). */
bool        zcsr_hsm_dispatch(zcsr_hsm*, const char* signal, const char* payload);
/* Current state name (NUL-terminated); also persisted to the state buffer. */
const char* zcsr_hsm_current(const zcsr_hsm*);

#endif /* ZCSR_HSM_H */
