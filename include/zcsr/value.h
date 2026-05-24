#ifndef ZCSR_VALUE_H
#define ZCSR_VALUE_H
/* Zero-Coupling Signal Router — shared value type (Agent 1 data primitive).
 * ONLY string / int / bool, per spec. No heap. C11/C17. */
#include <stdint.h>
#include <stdbool.h>

typedef enum { ZCSR_NONE = 0, ZCSR_BOOL, ZCSR_INT, ZCSR_STR } zcsr_value_type;

/* Tagged value. For ZCSR_STR, `s` is a NUL-terminated pointer into caller/arena-owned
 * storage; zcsr_value never owns or frees memory. */
typedef struct {
    zcsr_value_type type;
    bool            b;
    int64_t         i;
    const char*     s;
} zcsr_value;

static inline zcsr_value zcsr_none(void)         { zcsr_value v = { ZCSR_NONE, false, 0, 0 }; return v; }
static inline zcsr_value zcsr_bool(bool x)       { zcsr_value v = { ZCSR_BOOL, x,     0, 0 }; return v; }
static inline zcsr_value zcsr_int(int64_t x)     { zcsr_value v = { ZCSR_INT,  false, x, 0 }; return v; }
static inline zcsr_value zcsr_str(const char* x) { zcsr_value v = { ZCSR_STR,  false, 0, x }; return v; }

#endif /* ZCSR_VALUE_H */
