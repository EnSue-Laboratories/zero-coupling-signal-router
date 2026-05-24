#ifndef ZCSR_HSM_H
#define ZCSR_HSM_H
/* Agent 3 — Hierarchical State Machine. No global state. Transitions via signals only.
 * Persists current state to the Core state buffer. Implementer: modules/hsm. */
#include "state_buffer.h"
#include <stdbool.h>

typedef struct zcsr_hsm zcsr_hsm; /* opaque; instance-owned, no globals */

zcsr_hsm*   zcsr_hsm_create(zcsr_state* persist_to);
void        zcsr_hsm_destroy(zcsr_hsm*);
/* Deliver a signal to drive a transition. Returns true if a transition occurred. */
bool        zcsr_hsm_dispatch(zcsr_hsm*, const char* signal, const char* payload);
/* Current leaf-state name (NUL-terminated); also persisted to the state buffer. */
const char* zcsr_hsm_current(const zcsr_hsm*);

#endif /* ZCSR_HSM_H */
