/* Agent 2 — signal/slot dispatch. No heap, no dynamic registry.
 * This provides a working table-walk baseline so integration can dispatch today.
 * TODO(Agent 2 / Codex): add the ZCSR_ROUTER X-macro to declare the `static const zcsr_conn[]`
 * table at compile time (zero runtime registration), and keep tag matching minimal. */
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
