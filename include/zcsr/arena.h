#ifndef ZCSR_ARENA_H
#define ZCSR_ARENA_H
/* Agent 1 — stack-only memory manager (no runtime/heap allocation). Implementer: modules/core.
 * CONTRACT ONLY (opaque handle). Modules include this header, never another module's source. */
#include <stddef.h>

typedef struct zcsr_arena zcsr_arena; /* opaque */

/* Place a linear/stack allocator inside the caller's fixed buffer. No malloc ever.
 * Returns NULL if the buffer is too small to host the bookkeeping header. */
zcsr_arena* zcsr_arena_init(void* buffer, size_t bytes);
void*       zcsr_arena_alloc(zcsr_arena*, size_t bytes, size_t align); /* NULL if exhausted */
void        zcsr_arena_reset(zcsr_arena*);                             /* free all (LIFO) */
size_t      zcsr_arena_used(const zcsr_arena*);
size_t      zcsr_arena_capacity(const zcsr_arena*);

#endif /* ZCSR_ARENA_H */
