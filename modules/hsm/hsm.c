/* Agent 3 — Hierarchical State Machine. No global state. Transitions via signals only.
 * Persists current state into the Core state buffer (zcsr_state). */
#include "zcsr/hsm.h"

/* TODO(Agent 3 / Codex): implement zcsr_hsm
 *   - hierarchical states, instance-owned (no global/static mutable state).
 *   - dispatch(signal, payload): resolve a transition for the active state hierarchy.
 *   - on transition: zcsr_state_set(persist_to, "hsm.state", zcsr_str(current)). */

zcsr_hsm*   zcsr_hsm_create(zcsr_state* persist_to)                      { (void)persist_to; return 0; }
void        zcsr_hsm_destroy(zcsr_hsm* h)                               { (void)h; }
bool        zcsr_hsm_dispatch(zcsr_hsm* h, const char* sig, const char* p) { (void)h; (void)sig; (void)p; return false; }
const char* zcsr_hsm_current(const zcsr_hsm* h)                         { (void)h; return ""; }
