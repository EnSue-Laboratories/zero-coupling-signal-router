/* Regression: a same-key string slot stays reusable by its ALLOCATED capacity, so long->short->long
 * neither appends nor fails. Under the old strlen-based reuse this would append a fresh copy on every
 * grow-after-shrink and exhaust the (modest) string pool. */
#include "zcsr/state_buffer.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    /* Modest buffer: room for the key + ONE long value slot, NOT for ~hundreds of appended copies. */
    static unsigned char mem[2 * 1024];
    int fails = 0;
    zcsr_state* s = zcsr_state_init(mem, sizeof mem);
    const char* longv  = "active.timing.substate.long"; /* 27 chars */
    const char* shortv = "idle";

    if (!s) { printf("FAIL: init\n"); return 1; }

    if (!zcsr_state_set(s, "k", zcsr_str(longv))) { printf("FAIL: initial long set\n"); fails++; }

    /* Hammer long<->short; capacity-based reuse keeps strings_used flat and never fails. */
    for (int i = 0; i < 200 && !fails; ++i) {
        if (!zcsr_state_set(s, "k", zcsr_str(shortv))) { printf("FAIL: short set @%d (pool exhausted?)\n", i); fails++; }
        if (!zcsr_state_set(s, "k", zcsr_str(longv)))  { printf("FAIL: long set @%d (pool exhausted?)\n", i); fails++; }
    }

    {
        zcsr_value v = zcsr_state_get(s, "k");
        if (v.type != ZCSR_STR || !v.s || strcmp(v.s, longv) != 0) { printf("FAIL: final value mismatch\n"); fails++; }
    }

    if (!fails) printf("test_state_reuse: PASS (capacity-based slot reuse; no growth on shrink/grow)\n");
    return fails ? 1 : 0;
}
