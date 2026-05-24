#include "zcsr/arena.h"
#include "zcsr/state_buffer.h"

#include <stdint.h>

static int fail_if(int condition) {
    return condition ? 1 : 0;
}

int main(void) {
    unsigned char arena_mem[256];
    unsigned char state_mem[4096];
    zcsr_arena* arena = zcsr_arena_init(arena_mem, sizeof arena_mem);
    zcsr_state* state = zcsr_state_init(state_mem, sizeof state_mem);
    void* a;
    void* b;
    zcsr_value v;

    if (fail_if(!arena || !state)) return 1;
    if (fail_if(zcsr_arena_used(arena) != 0)) return 2;
    if (fail_if(zcsr_arena_capacity(arena) == 0)) return 3;

    a = zcsr_arena_alloc(arena, 7, 8);
    b = zcsr_arena_alloc(arena, 3, 16);
    if (fail_if(!a || !b)) return 4;
    if (fail_if(((uintptr_t)a % 8u) != 0u)) return 5;
    if (fail_if(((uintptr_t)b % 16u) != 0u)) return 6;
    if (fail_if(zcsr_arena_used(arena) == 0)) return 7;
    zcsr_arena_reset(arena);
    if (fail_if(zcsr_arena_used(arena) != 0)) return 8;

    if (fail_if(!zcsr_state_set(state, "visible", zcsr_bool(true)))) return 9;
    if (fail_if(!zcsr_state_set(state, "count", zcsr_int(42)))) return 10;
    if (fail_if(!zcsr_state_set(state, "label", zcsr_str("potion")))) return 11;
    if (fail_if(!zcsr_state_has(state, "visible"))) return 12;

    v = zcsr_state_get(state, "visible");
    if (fail_if(v.type != ZCSR_BOOL || !v.b)) return 13;
    v = zcsr_state_get(state, "count");
    if (fail_if(v.type != ZCSR_INT || v.i != 42)) return 14;
    v = zcsr_state_get(state, "label");
    if (fail_if(v.type != ZCSR_STR || !v.s || v.s[0] != 'p')) return 15;

    if (fail_if(!zcsr_state_set(state, "count", zcsr_int(43)))) return 16;
    v = zcsr_state_get(state, "count");
    if (fail_if(v.type != ZCSR_INT || v.i != 43)) return 17;
    if (fail_if(zcsr_state_get(state, "missing").type != ZCSR_NONE)) return 18;

    return 0;
}
