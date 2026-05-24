#ifndef ZCSR_STATE_BUFFER_H
#define ZCSR_STATE_BUFFER_H
/* Agent 1 — read-only game state buffer (string/int/bool). Implementer: modules/core.
 * Reads take `const zcsr_state*`; only the privileged writer mutates. No heap growth. */
#include "value.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct zcsr_state zcsr_state; /* opaque */

zcsr_state* zcsr_state_init(void* buffer, size_t bytes);

/* read side (const) */
zcsr_value  zcsr_state_get(const zcsr_state*, const char* key);
bool        zcsr_state_has(const zcsr_state*, const char* key);

/* write side (privileged) — false when fixed capacity is exceeded; never grows the heap */
bool        zcsr_state_set(zcsr_state*, const char* key, zcsr_value v);

#endif /* ZCSR_STATE_BUFFER_H */
