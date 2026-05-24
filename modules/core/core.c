/* Agent 1 — Core Foundation (stack-only memory manager + read-only state buffer).
 * Constraints: no runtime allocation, string/int/bool only, zero deps, footprint < 256KB.
 * Includes ONLY shared contracts — never another module's source. */
#include "zcsr/arena.h"
#include "zcsr/state_buffer.h"

/* TODO(Agent 1 / Codex): implement
 *   - zcsr_arena: linear/stack allocator over the caller buffer (no malloc).
 *   - zcsr_state: fixed-capacity key->zcsr_value store (string/int/bool), no heap. */

zcsr_arena* zcsr_arena_init(void* buffer, size_t bytes)            { (void)buffer; (void)bytes; return 0; }
void*       zcsr_arena_alloc(zcsr_arena* a, size_t bytes, size_t align) { (void)a; (void)bytes; (void)align; return 0; }
void        zcsr_arena_reset(zcsr_arena* a)                        { (void)a; }
size_t      zcsr_arena_used(const zcsr_arena* a)                   { (void)a; return 0; }
size_t      zcsr_arena_capacity(const zcsr_arena* a)               { (void)a; return 0; }

zcsr_state* zcsr_state_init(void* buffer, size_t bytes)            { (void)buffer; (void)bytes; return 0; }
zcsr_value  zcsr_state_get(const zcsr_state* s, const char* key)   { (void)s; (void)key; return zcsr_none(); }
bool        zcsr_state_has(const zcsr_state* s, const char* key)   { (void)s; (void)key; return false; }
bool        zcsr_state_set(zcsr_state* s, const char* key, zcsr_value v) { (void)s; (void)key; (void)v; return false; }
