/* Agent 2 — signal/slot dispatch. No heap, no dynamic registry.
 * Runtime over a compile-time `static const zcsr_conn[]` table (declared via the
 * ZCSR_DEFINE_ROUTER X-macro in zcsr/signal.h). This TU holds the dispatch walk. */
#include "zcsr/signal.h"
#include <string.h>

void zcsr_emit(const zcsr_conn* table, size_t count, const char* signal, const char* payload) {
    if (!table || !signal) return;
    for (size_t k = 0; k < count; ++k) {
        if (table[k].signal && table[k].slot && strcmp(table[k].signal, signal) == 0) {
            table[k].slot(payload);
        }
    }
}
